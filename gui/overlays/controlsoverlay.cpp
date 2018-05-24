#include "controlsoverlay.h"

ControlsOverlay::ControlsOverlay(OverlayContainerWidget *parent) :
    OverlayWidget(parent)
{
    settingsButton = new IconButton("openSettings", ":/res/icons/buttons/settings20.png");
    closeButton = new IconButton("exit", ":/res/icons/buttons/close20.png");
    layout.setContentsMargins(0,0,0,0);
    this->setContentsMargins(0,0,0,0);
    layout.setSpacing(0);
    layout.addWidget(settingsButton);
    layout.addWidget(closeButton);
    setLayout(&layout);
    fitToContents();

    setMouseTracking(true);

    fadeEffect = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(fadeEffect);
    fadeAnimation = new QPropertyAnimation(fadeEffect, "opacity");
    fadeAnimation->setDuration(230);
    fadeAnimation->setStartValue(1.0f);
    fadeAnimation->setEndValue(0);
    fadeAnimation->setEasingCurve(QEasingCurve::OutQuart);
    this->show();
}

void ControlsOverlay::show() {
    fadeEffect->setOpacity(0.0f);
    OverlayWidget::show();
}

QSize ControlsOverlay::contentsSize() {
    QSize newSize(0, 0);
    for(int i=0; i<layout.count(); i++) {
        newSize.setWidth(newSize.width() + layout.itemAt(i)->widget()->width());
        newSize.setHeight(layout.itemAt(i)->widget()->height());
    }
    return newSize;
}

void ControlsOverlay::fitToContents() {
    this->setFixedSize(contentsSize());
    recalculateGeometry();
}

void ControlsOverlay::recalculateGeometry() {
    setGeometry(containerSize().width() - width(), 0, width(), height());
}

void ControlsOverlay::enterEvent(QEvent *event) {
    fadeAnimation->stop();
    fadeEffect->setOpacity(1.0f);
}

void ControlsOverlay::leaveEvent(QEvent *event) {
    fadeAnimation->start();
}
