#include "thumbnailview.h"

ThumbnailView::ThumbnailView(ThumbnailViewOrientation orient, QWidget *parent)
    : QGraphicsView(parent),
      orientation(orient),
      blockThumbnailLoading(false),
      mSelectedIndex(-1),
      mDrawScrollbarIndicator(true),
      mCropThumbnails(false),
      scrollTimeLine(nullptr),
      mThumbnailSize(120)
{
    setAccessibleName("thumbnailView");
    this->setMouseTracking(true);
    this->setScene(&scene);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    this->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    this->setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);

    createScrollTimeLine();

    connect(&loadTimer, &QTimer::timeout, this, &ThumbnailView::loadVisibleThumbnails);
    loadTimer.setInterval(static_cast<const int>(LOAD_DELAY));
    loadTimer.setSingleShot(true);

    if(orientation == THUMBNAILVIEW_HORIZONTAL) {
        this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollBar = this->horizontalScrollBar();
        connect(scrollTimeLine, &QTimeLine::frameChanged, this, &ThumbnailView::centerOnX);
    } else {
        this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollBar = this->verticalScrollBar();
        connect(scrollTimeLine, &QTimeLine::frameChanged, this, &ThumbnailView::centerOnY);
    }

    scrollBar->setContextMenuPolicy(Qt::NoContextMenu);
    scrollBar->installEventFilter(this);

    connect(scrollBar, &QScrollBar::valueChanged, [this]() {
        loadVisibleThumbnails();
    });
}

void ThumbnailView::createScrollTimeLine() {
    if(scrollTimeLine) {
        scrollTimeLine->stop();
        scrollTimeLine->deleteLater();
    }
    /* scrolling-related things */
    scrollTimeLine = new QTimeLine(SCROLL_ANIMATION_SPEED, this);
    scrollTimeLine->setEasingCurve(QEasingCurve::OutSine);
    scrollTimeLine->setUpdateInterval(SCROLL_UPDATE_RATE);
    if(orientation == THUMBNAILVIEW_HORIZONTAL) {
        connect(scrollTimeLine, &QTimeLine::frameChanged, this, &ThumbnailView::centerOnX);
    } else {
        connect(scrollTimeLine, &QTimeLine::frameChanged, this, &ThumbnailView::centerOnY);
    }
    connect(scrollTimeLine, &QTimeLine::finished, [this]() {
        blockThumbnailLoading = false;
        loadVisibleThumbnails();
    });
}

bool ThumbnailView::eventFilter(QObject *o, QEvent *ev) {
    if (o == scrollBar) {
        if(ev->type() == QEvent::Wheel) {
            this->wheelEvent(dynamic_cast<QWheelEvent*>(ev));
            return true;
        } else if(ev->type() == QEvent::Paint && mDrawScrollbarIndicator) {
            QPainter p(scrollBar);
            p.setOpacity(0.3f);
            p.fillRect(indicator, QBrush(Qt::gray));
            p.setOpacity(1.0f);
            return false;
        }

    }
    return QObject::eventFilter(o, ev);
}

void ThumbnailView::setDirectoryPath(QString path) {
    Q_UNUSED(path)
}

void ThumbnailView::selectIndex(int index) {
    if(!checkRange(index))
        return;

    if(checkRange(mSelectedIndex))
        thumbnails.at(mSelectedIndex)->setHighlighted(false);

    mSelectedIndex = index;

    ThumbnailWidget *thumb = thumbnails.at(mSelectedIndex);
    thumb->setHighlighted(true);
    updateScrollbarIndicator();
}

int ThumbnailView::selectedIndex() {
    return mSelectedIndex;
}

int ThumbnailView::itemCount() {
    return thumbnails.count();
}

void ThumbnailView::showEvent(QShowEvent *event) {
    QGraphicsView::showEvent(event);
    // ensure we are properly resized
    qApp->processEvents();
    updateScrollbarIndicator();
    ensureSelectedItemVisible();
    loadVisibleThumbnails();
}

void ThumbnailView::populate(int count) {
    if(count >= 0) {
        // reuse existing items
        if(count == thumbnails.count()) {
            QList<ThumbnailWidget*>::iterator i;
            for (i = thumbnails.begin(); i != thumbnails.end(); ++i) {
                (*i)->reset();
            }
        } else {
            removeAll();
            for(int i = 0; i < count; i++) {
                ThumbnailWidget *widget = createThumbnailWidget();
                widget->setThumbnailSize(mThumbnailSize);
                thumbnails.append(widget);
                addItemToLayout(widget, i);
            }
        }
    }
    mSelectedIndex = -1;
    updateLayout();
    fitSceneToContents();
    resetViewport();
}

void ThumbnailView::addItem() {
    insertItem(thumbnails.count());
}

// insert at index
void ThumbnailView::insertItem(int index) {
    if(index <= mSelectedIndex) {
        mSelectedIndex++;
    }
    ThumbnailWidget *widget = createThumbnailWidget();
    thumbnails.insert(index, widget);
    addItemToLayout(widget, index);
    updateLayout();
    fitSceneToContents();
    updateScrollbarIndicator();
    loadVisibleThumbnails();
}

void ThumbnailView::removeItem(int index) {
    if(checkRange(index)) {
        removeItemFromLayout(index);
        delete thumbnails.takeAt(index);
        fitSceneToContents();
        if(index < mSelectedIndex) {
            selectIndex(mSelectedIndex - 1);
        } else if(index == mSelectedIndex) {
            if(mSelectedIndex >= thumbnails.count())
                selectIndex(thumbnails.count() - 1);
            else
                selectIndex(mSelectedIndex);
        }
        updateScrollbarIndicator();
        loadVisibleThumbnails();
    }
}

void ThumbnailView::reloadItem(int index) {
    if(!checkRange(index))
        return;
    auto thumb = thumbnails.at(index);
    if(thumb->isLoaded) {
        thumb->unsetThumbnail();
        emit thumbnailsRequested(QList<int>() << index, static_cast<int>(qApp->devicePixelRatio() * mThumbnailSize), mCropThumbnails, true);
    }
}

void ThumbnailView::setCropThumbnails(bool mode) {
    if(mode != mCropThumbnails) {
        unloadAllThumbnails();
        mCropThumbnails = mode;
        loadVisibleThumbnails();
    }
}

void ThumbnailView::setDrawScrollbarIndicator(bool mode) {
    mDrawScrollbarIndicator = mode;
}

void ThumbnailView::setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) {
    if(thumb && thumb->size() == floor(mThumbnailSize * qApp->devicePixelRatio()) && checkRange(pos)) {
        thumbnails.at(pos)->setThumbnail(thumb);
    }
}

void ThumbnailView::unloadAllThumbnails() {
    for(int i = 0; i < thumbnails.count(); i++) {
        thumbnails.at(i)->unsetThumbnail();
    }
}

void ThumbnailView::loadVisibleThumbnails() {
    loadTimer.stop();
    if(isVisible() && !blockThumbnailLoading) {
        QRectF visibleRect = mapToScene(viewport()->geometry()).boundingRect();
        // grow rectangle to cover nearby offscreen items
        visibleRect.adjust(-offscreenPreloadArea, -offscreenPreloadArea,
                           offscreenPreloadArea, offscreenPreloadArea);
        QList<QGraphicsItem *>visibleItems = scene.items(visibleRect,
                                                         Qt::IntersectsItemShape,
                                                         Qt::AscendingOrder);
        // load new previews
        QList<int> loadList;
        for(int i = 0; i < visibleItems.count(); i++) {
            ThumbnailWidget* widget = qgraphicsitem_cast<ThumbnailWidget*>(visibleItems.at(i));
            if(!widget->isLoaded) {
                loadList.append(thumbnails.indexOf(widget));
            }
        }
        if(loadList.count()) {
            emit thumbnailsRequested(loadList, static_cast<int>(qApp->devicePixelRatio() * mThumbnailSize), mCropThumbnails, false);
        }
        // unload offscreen
        for(int i = 0; i < thumbnails.count(); i++) {
            if(!visibleItems.contains(thumbnails.at(i))) {
                thumbnails.at(i)->unsetThumbnail();
            }
        }
    }
}

void ThumbnailView::loadVisibleThumbnailsDelayed() {
    loadTimer.stop();
    loadTimer.start();
}

void ThumbnailView::centerOnX(int dx) {
    centerOn(dx + 1, viewportCenter.y());
    // trigger repaint immediately
    qApp->processEvents();
}

void ThumbnailView::centerOnY(int dy) {
    centerOn(viewportCenter.x(), dy + 1);
    // trigger repaint immediately
    qApp->processEvents();
}

void ThumbnailView::resetViewport() {
    if(scrollTimeLine->state() == QTimeLine::Running)
        scrollTimeLine->stop();
    scrollBar->setValue(0);
}

int ThumbnailView::thumbnailSize() {
    return mThumbnailSize;
}

bool ThumbnailView::atSceneStart() {
    if(orientation == THUMBNAILVIEW_HORIZONTAL) {
        if(viewportTransform().dx() == 0.0)
            return true;
    } else {
        if(viewportTransform().dy() == 0.0)
            return true;
    }
    return false;
}

bool ThumbnailView::atSceneEnd() {
    if(orientation == THUMBNAILVIEW_HORIZONTAL) {
        if(viewportTransform().dx() == viewport()->width() - sceneRect().width())
            return true;
    } else {
        if(viewportTransform().dy() == viewport()->height() - sceneRect().height())
            return true;
    }
    return false;
}

bool ThumbnailView::checkRange(int pos) {
    return pos >= 0 && pos < thumbnails.count();
}

void ThumbnailView::updateLayout() {

}

// fit scene to it's contents size
void ThumbnailView::fitSceneToContents() {
    scene.setSceneRect(scene.itemsBoundingRect());
}

//################### scrolling ######################
void ThumbnailView::wheelEvent(QWheelEvent *event) {
    event->accept();
    // alright, i officially gave up on fixing libinput scrolling
    // let's just hope it gets done in Qt while I am still alive
    int pixelDelta = event->pixelDelta().y();
    int angleDelta = event->angleDelta().ry();
    if(!settings->enableSmoothScroll()) {
        if(pixelDelta)
            scrollPrecise(pixelDelta);
        else if(angleDelta)
            scrollPrecise(angleDelta);
    } else {
        if(pixelDelta)
            scrollPrecise(pixelDelta);
        else if(abs(angleDelta) < SMOOTH_SCROLL_THRESHOLD)
            scrollPrecise(angleDelta);
        else if(angleDelta)
            scrollSmooth(angleDelta, SCROLL_SPEED_MULTIPLIER, SCROLL_SPEED_ACCELERATION, true);
    }
}

void ThumbnailView::scrollPrecise(int delta) {
    viewportCenter = mapToScene(viewport()->rect().center());
    if(scrollTimeLine->state() == QTimeLine::Running) {
        scrollTimeLine->stop();
        blockThumbnailLoading = false;
    }
    // ignore if we reached boundaries
    if( (delta > 0 && atSceneStart()) || (delta < 0 && atSceneEnd()) )
        return;
    // pixel scrolling (precise)
    if(orientation == THUMBNAILVIEW_HORIZONTAL) {
        centerOnX(static_cast<int>(viewportCenter.x() - delta));
    } else {
        centerOnY(static_cast<int>(viewportCenter.y() - delta));
    }
}

void ThumbnailView::scrollSmooth(int delta, qreal multiplier, qreal acceleration, bool additive) {
    viewportCenter = mapToScene(viewport()->rect().center());
    // ignore if we reached boundaries
    if( (delta > 0 && atSceneStart()) || (delta < 0 && atSceneEnd()) ) {
        return;
    }
    int center;
    if(orientation == THUMBNAILVIEW_HORIZONTAL)
        center = static_cast<int>(viewportCenter.x());
    else
        center = static_cast<int>(viewportCenter.y());
    bool redirect = false;
    int newEndFrame = center - static_cast<int>(delta * multiplier);
    if( (newEndFrame < center && center < scrollTimeLine->endFrame()) ||
        (newEndFrame > center && center > scrollTimeLine->endFrame()) )
    {
        redirect = true;
    }
    if(scrollTimeLine->state() == QTimeLine::Running) {
        int oldEndFrame = scrollTimeLine->endFrame();
        // QTimeLine has this weird issue when it is already finished (at the last frame)
        // but is stuck in the running state. So we just create a new one.
        if(oldEndFrame == center)
            createScrollTimeLine();
        if(!redirect && additive)
            newEndFrame = oldEndFrame - static_cast<int>(delta * multiplier * acceleration);
    }
    scrollTimeLine->stop();
    //blockThumbnailLoading = true;
    scrollTimeLine->setFrameRange(center, newEndFrame);
    scrollTimeLine->start();
}

void ThumbnailView::scrollSmooth(int delta, qreal multiplier, qreal acceleration) {
    scrollSmooth(delta, multiplier, acceleration, false);
}

void ThumbnailView::scrollSmooth(int angleDelta) {
    scrollSmooth(angleDelta, 1.0, 1.0, false);
}

void ThumbnailView::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->pos()));
        if(item) {
            emit thumbnailPressed(thumbnails.indexOf(item));
            return;
        }
    }
    event->ignore();
}

void ThumbnailView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    updateScrollbarIndicator();
}
