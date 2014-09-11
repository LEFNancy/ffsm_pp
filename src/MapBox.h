/***************************************************************************
 *   Copyright (C) 2012 by Laboratoire d'Economie Forestière               *
 *   http://www.nancy.inra.fr/lef_eng                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef MAPBOX_H
#define MAPBOX_H

#include <QColor> //TO.DO del
#include <QImage> //TO.DO del

#include <QWidget>
#include <QPixmap>


using namespace std;

/// Widget to display the maps of various spacial aspects of the model.

/**
This class is based on QImage. It pick-ups from layersVector the choosed layer and display it.
<br>It has methods to change the individual pixels or the whole image of a layer.
*/

class MapBox : public QWidget {
    Q_OBJECT

public:
                        MapBox(QWidget *parent = 0);
  int                 getLayerIndex(QString layerName_h=""); ///< Return the index of the specified layer (null to ask for the current one)

public slots:
  void                updatePixel(QString layerName_h, int x_h, int y_h, QColor color_h);
  void                updateImage(QString layerName_h, const QImage& image_h);
  void                switchToLayer(QString layerName_h); ///< Change the layer that currentLayer and currentLayerName points
  void                addLayer(QString layerName_h);
  void                fitInWindow();
  void                zoom(double zoomFactor);
  void                scroll(int deltaX, int deltaY);

signals:
  void                queryRequestOnPx(int px_ID, int currentLayerIndex, bool newRequest);

private:
  void                updatePixmap(const QImage &image, bool reFit=false);
  void                paintEvent(QPaintEvent *event); ///< Reimplementation of the standard paintEvent method.
  void                prepareQueryEvent(QPoint click);
  void                keyPressEvent(QKeyEvent *event);
  void                wheelEvent(QWheelEvent *event);
  void                mousePressEvent(QMouseEvent *event);
  void                mouseMoveEvent(QMouseEvent *event);
  vector <QImage>                 layersVector; ///< Vector of QImages
  vector <QString>            layersNameVector; ///< Vector of layer names
  QImage                          currentLayer;
  QString                     currentLayerName;
  QPoint lastDragPos;
  double sx1, sy1, sx2, sy2; ///< coordinates of corner pixels of source - pixmap - rectangle
  double dx1, dy1, dx2, dy2; ///< coordinates of corner pixels of destination - widget - rectangle  

};

#endif
