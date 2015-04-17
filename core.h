#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QWidget>
#include <QFileDialog>
#include "directorymanager.h"
#include "imageloader.h"
#include "settings.h"

class Core : public QObject
{
    Q_OBJECT
public:
    explicit Core();
    void open(QString);
    const ImageCache* getCache();

private:
    void initVariables();
    void connectSlots();
    ImageLoader *imageLoader;
    DirectoryManager *dirManager;
    Image* currentImage;

private slots:
    void onLoadFinished(Image *img);

    void crop(QRect newRect);
signals:
    void signalUnsetImage();
    void signalSetImage(Image*);
    void infoStringChanged(QString);
    void slowLoading();
    void imageAltered();
    void directoryChanged(QString);

public slots:
    void updateInfoString();
    void loadImage(QString);
    void initSettings();
    void slotNextImage();
    void slotPrevImage();
    void setCurrentDir(QString);
    void rotateImage(int);

    void saveImage(QString path);
    void saveImage();
    void loadImageByPos(int pos);
};

#endif // CORE_H

