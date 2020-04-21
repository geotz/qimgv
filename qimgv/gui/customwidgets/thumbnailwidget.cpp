#include "thumbnailwidget.h"

ThumbnailWidget::ThumbnailWidget(QGraphicsItem *parent) :
    QGraphicsWidget(parent),
    isLoaded(false),
    thumbnail(nullptr),
    highlighted(false),
    hovered(false),
    mDrawLabel(true),
    mThumbnailSize(100),
    paddingX(1),
    paddingY(3),
    textHeight(5)
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    // ! this breaks HiDPI !
    //setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setAcceptHoverEvents(true);
    nameColor.setRgb(20, 20, 20, 255);
    qreal fntSz = font.pointSizeF();
    if(fntSz > 0) {
        //font.setPointSizeF(font.pointSizeF() * 0.9);
        fontSmall.setPointSizeF(fontSmall.pointSizeF() * 0.85);
    }
    font.setBold(true);
    fontSmall.setBold(true);
    fm = new QFontMetrics(font);
    fmSmall = new QFontMetrics(fontSmall);
    textHeight = fm->height();

    setThumbnailSize(100);
    readSettings();
    connect(settings, &Settings::settingsChanged, this, &ThumbnailWidget::readSettings);
}

void ThumbnailWidget::readSettings() {
    highlightColor.setRgb(settings->accentColor().rgb());
}

void ThumbnailWidget::setThumbnailSize(int size) {
    if(mThumbnailSize != size && size > 0) {
        isLoaded = false;
        mThumbnailSize = size;
        updateGeometry();
        updateThumbnailDrawPosition();
        updateHighlightRect();
        setupLayout();
        update();
    }
}

void ThumbnailWidget::setPadding(int x, int y) {
    paddingX = x;
    paddingY = y;
}

int ThumbnailWidget::thumbnailSize() {
    return mThumbnailSize;
}

void ThumbnailWidget::reset() {
    if(thumbnail)
        thumbnail.reset();
    highlighted = false;
    hovered = false;
    isLoaded = false;
    update();
}

void ThumbnailWidget::setDrawLabel(bool mode) {
    if(mDrawLabel != mode) {
        mDrawLabel = mode;
        updateThumbnailDrawPosition();
        setupLayout();
        updateHighlightRect();
        updateGeometry();
        update();
    }
}

void ThumbnailWidget::updateGeometry() {
    QGraphicsWidget::updateGeometry();
}

void ThumbnailWidget::setGeometry(const QRectF &rect) {
    QGraphicsWidget::setGeometry(QRectF(rect.topLeft(), boundingRect().size()));
}

QRectF ThumbnailWidget::geometry() const {
    return QRectF(QGraphicsWidget::geometry().topLeft(), boundingRect().size());
}

QSizeF ThumbnailWidget::effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint) const {
    return sizeHint(which, constraint);
}

void ThumbnailWidget::setThumbnail(std::shared_ptr<Thumbnail> _thumbnail) {
    if(_thumbnail) {
        thumbnail = _thumbnail;
        isLoaded = true;
        updateThumbnailDrawPosition();
        setupLayout();
        updateHighlightRect();
        update();
    }
}

void ThumbnailWidget::unsetThumbnail() {
    if(thumbnail)
        thumbnail.reset();
    isLoaded = false;
    /*
    updateThumbnailDrawPosition();
    setupLayout();
    updateHighlightRect();
    update();
    */
}

void ThumbnailWidget::setupLayout() {
    nameRect = QRectF(highlightRect.left(), highlightRect.height(),
                      highlightRect.width(), textHeight * 1.7);

    if(thumbnail && !thumbnail->label().isEmpty()) {
        nameTextRect = nameRect.adjusted(4, 0, -4, 0);
	labelTextRect.setWidth(fmSmall->boundingRect(thumbnail->label()).width());
        labelTextRect.setHeight(nameRect.height());
        labelTextRect.moveTop(nameTextRect.top());
        labelTextRect.moveRight(nameTextRect.right());
        nameTextRect.adjust(0, 0, -labelTextRect.width() - 4, 0);
    }
}

void ThumbnailWidget::updateHighlightRect() {
    highlightRect = QRectF(paddingX, 0, width() - paddingX * 2, paddingY);
}

void ThumbnailWidget::setHighlighted(bool mode) {
    if(highlighted != mode) {
        highlighted = mode;
        update();
    }
}

bool ThumbnailWidget::isHighlighted() {
    return highlighted;
}

QRectF ThumbnailWidget::boundingRect() const {
    return QRectF(0, 0, mThumbnailSize + paddingX * 2, mThumbnailSize + paddingY * 2);
}

qreal ThumbnailWidget::width() {
    return boundingRect().width();
}

qreal ThumbnailWidget::height() {
    return boundingRect().height();
}

void ThumbnailWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(widget)
    Q_UNUSED(option)
    qreal dpr = painter->paintEngine()->paintDevice()->devicePixelRatioF();
    if(isHighlighted())
        drawHighlight(painter);
    if(!thumbnail) {
        QPixmap* loadingIcon = shrRes->getPixmap(ShrIcon::SHR_ICON_LOADING, dpr);
        drawIcon(painter, dpr, loadingIcon);
    } else {
        if(thumbnail->pixmap().get()->width() == 0) {
            QPixmap* errorIcon = shrRes->getPixmap(ShrIcon::SHR_ICON_ERROR, dpr);
            drawIcon(painter, dpr, errorIcon);
        } else {
            drawThumbnail(painter, dpr, thumbnail->pixmap().get());
        }
        if(mDrawLabel)
            drawLabel(painter);
    }
    if(isHovered())
        drawHover(painter);
}

void ThumbnailWidget::drawHighlight(QPainter *painter) {
    painter->fillRect(highlightRect, highlightColor);
}

void ThumbnailWidget::drawHover(QPainter *painter) {
    painter->setOpacity(0.6);
    painter->fillRect(highlightRect, highlightColor);
    painter->setOpacity(1.0);
    painter->fillRect(boundingRect(), QColor(255,255,255, 10));
}

void ThumbnailWidget::drawLabel(QPainter *painter) {
    // text background
    painter->setOpacity(0.95);
    painter->fillRect(nameRect, nameColor);
    painter->setOpacity(1.0);
    // filename
    int flags = Qt::TextSingleLine | Qt::AlignVCenter;
    painter->setFont(font);
    painter->setPen(QColor(210, 210, 210, 255));
    painter->drawText(nameTextRect, flags, thumbnail->name());
    // additional info
    painter->setFont(fontSmall);
    painter->setPen(QColor(160, 160, 160, 255));
    painter->drawText(labelTextRect, flags, thumbnail->label());
}

void ThumbnailWidget::drawThumbnail(QPainter* painter, qreal /*dpr*/, const QPixmap *pixmap) {
    painter->drawPixmap(drawRectCentered, *pixmap);
}

void ThumbnailWidget::drawIcon(QPainter* painter, qreal /*dpr*/, const QPixmap *pixmap) {
    QPointF pos = QPointF(width()  / 2 - pixmap->width()  / (2 * pixmap->devicePixelRatioF()),
                          height() / 2 - pixmap->height() / (2 * pixmap->devicePixelRatioF()));
    painter->drawPixmap(pos, *pixmap);
}

QSizeF ThumbnailWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const {
    Q_UNUSED(which);
    Q_UNUSED(constraint);

    return boundingRect().size();
}

void ThumbnailWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    event->ignore();
    setHovered(true);
}

void ThumbnailWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    event->ignore();
    setHovered(false);
}

void ThumbnailWidget::setHovered(bool mode) {
    if(hovered != mode) {
        hovered = mode;
        update();
    }
}

bool ThumbnailWidget::isHovered() {
    return hovered;
}

void ThumbnailWidget::updateThumbnailDrawPosition() {
    if(thumbnail) {
        qreal dpr = qApp->devicePixelRatio();
        if(isLoaded) {
            // correctly sized thumbnail
            QPoint topLeft(width()  / 2 - thumbnail->pixmap()->width()  / (2 * dpr),
                           height() / 2 - thumbnail->pixmap()->height() / (2 * dpr));
            drawRectCentered = QRect(topLeft, thumbnail->pixmap()->size() / dpr);
        } else {
            // old size pixmap, scaling
            QSize scaled = thumbnail->pixmap()->size().scaled(mThumbnailSize, mThumbnailSize, Qt::KeepAspectRatioByExpanding);
            QPoint topLeft(width()  / 2 - scaled.width()  / (2 * dpr),
                           height() / 2 - scaled.height() / (2 * dpr));
            drawRectCentered = QRect(topLeft, scaled);
        }
    }
}

ThumbnailWidget::~ThumbnailWidget() {
    delete fm;
    delete fmSmall;
}
