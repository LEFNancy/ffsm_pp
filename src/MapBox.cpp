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
#include <iostream>

//#include <QtGui>   // Qt4
#include <QtWidgets> // Qt5
#include <math.h>
#include "MapBox.h"

using namespace std;

const double ZoomOutFactor = 0.8;
const double ZoomInFactor = 1 / ZoomOutFactor;
const int ScrollStep = 20;

MapBox::MapBox(QWidget *parent):QWidget(parent) {

  currentLayerName = "";
  setCursor(Qt::CrossCursor);

  // setting source and destination init corners..
  sx1 = 0;
  sy1 = 0;
  sx2 = this->width();
  sy2 = this->height();
  dx1 = 0;
  dy1 = 0;
  dx2 = this->width();
  dy2 = this->height();
}

/**
We paint the image pixel by pixel picking up the colors from the map pointed by currentLayer.
*/
void
MapBox::paintEvent(QPaintEvent *event) {

  if (layersVector.size() < 1) return;
  QPainter painter(this);
  painter.fillRect(rect(), Qt::lightGray  );
  QPixmap pixmap = QPixmap::fromImage(currentLayer); // It doesn't get autmoatically refreshed if I use a separate function to update the pixmap from the image
  QRectF source     (sx1, sy1, sx2-sx1, sy2-sy1); // the second point is in coordinates origin of the firt point !!!!
  QRectF destination(dx1, dy1, dx2-dx1, dy2-dy1); // the second point is in coordinates origin of the firt point !!!!
  /*
  This is the main function of the widget... the good pointa are:
  A) It takes into account the low level details of scaling, such interpolation
  B) If the destination is outside the widgets bounds, it doesn't matter. It make its job on the widget without any error (in this sence it isnot like an array luckily...)
  */
  painter.drawPixmap(destination, pixmap, source);

}

void
MapBox::updatePixel(QString layerName_h, int x_h, int y_h, QColor color_h){
  for (uint i=0;i<layersVector.size();i++){
    if (layersNameVector.at(i) == layerName_h){
      layersVector.at(i).setPixel(x_h, y_h, color_h.rgb());
      if(layerName_h == currentLayerName){
        currentLayer = layersVector.at(i);
        update();
      }
      return;
    }
  }
}

void
MapBox::updateImage(QString layerName_h, const QImage& image_h){
  static int counter = 0;
  for (uint i=0;i<layersVector.size();i++){
    if (layersNameVector.at(i) == layerName_h){
      layersVector.at(i) = image_h;
      if(layerName_h == currentLayerName){
        currentLayer = layersVector.at(i);
        update();
      }
      if (counter == 0) { // that's the first image we got !!
        fitInWindow();
      }
      counter ++;
      return;
    }
  }
  counter ++;
  cout << "*** ERROR in MapBox::updateImage(): layerName_h "<< qPrintable(layerName_h) << " not found "<< endl;
}

void
MapBox::switchToLayer(QString layerName_h){
  if (layerName_h != currentLayerName){
    for (uint i=0;i<layersVector.size();i++){
      if (layersNameVector.at(i) == layerName_h){
        currentLayer = layersVector.at(i);
        currentLayerName = layerName_h;
        update();
        return;
      }
    }
    cout << "*** ERROR in MapBox::switchToLayer(): layerName_h "<< qPrintable(layerName_h) << " not found "<< endl;
  }
}

int
MapBox::getLayerIndex(QString layerName_h){
  if( layerName_h == "") layerName_h = currentLayerName;
  for (uint i=0;i<layersVector.size();i++){
    if (layersNameVector.at(i) == layerName_h){
      return i;
    }
  }
  cout << "*** ERROR in MapBox:getLayerIndex(): layerName_h "<< qPrintable(layerName_h) << " not found "<< endl;
  return -1;
}

void
MapBox::addLayer(QString layerName_h){
  static int counter = 0;
  QImage newlayer = QImage(this->width(), this->height(), QImage::Format_RGB32);
  newlayer.fill(qRgb(255, 255, 255));
  layersVector.push_back(newlayer);
  layersNameVector.push_back(layerName_h);
  if (counter == 0) {
    currentLayerName = layerName_h;
    currentLayer = layersVector.at(0);
  }
  counter ++;
}

void
MapBox::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
    case Qt::Key_Plus:
    zoom(ZoomInFactor);
    break;
  case Qt::Key_Minus:
    zoom(ZoomOutFactor);
    break;
  case Qt::Key_Left:
    scroll(+ScrollStep, 0);
    break;
  case Qt::Key_Right:
    scroll(-ScrollStep, 0);
    break;
  case Qt::Key_Down:
    scroll(0, -ScrollStep);
    break;
  case Qt::Key_Up:
    scroll(0, +ScrollStep);
    break;
  default:
    QWidget::keyPressEvent(event);
  }
}

void
MapBox::wheelEvent(QWheelEvent *event){
  int numDegrees = event->delta() / 8;
  double numSteps = numDegrees / 15.0f;
  zoom(pow(ZoomInFactor, numSteps));
}

void
MapBox::mousePressEvent(QMouseEvent *event){
  if (event->button() == Qt::LeftButton){
    lastDragPos = event->pos();
  }
  else if (event->button() == Qt::RightButton){
    prepareQueryEvent(event->pos());
  }
}

void
MapBox::prepareQueryEvent(QPoint click){
  double cx = ((double) click.x()); //clicked x, casted to double
  double cy = ((double) click.y()); //clicked y, casted to double
  int   mx, my = 0; // outputed x and y
  int   px_ID;  // pixel ID
  int   layerIndex = getLayerIndex();
  // checking it is not out of the destination border range..
  if (cx>dx2 || cx<dx1 || cy>dy2 || cy<dy1) return;
  mx = ( (int) (cx-dx1) * (sx2-sx1)/(dx2-dx1) + sx1); // casting to int, not round() !!
  my = ( (int) (cy-dy1) * (sy2-sy1)/(dy2-dy1) + sy1); // casting to int, not round() !!
  px_ID = mx+my*(sx2-sx1);
  emit queryRequestOnPx(px_ID, layerIndex, true);

}


void
MapBox::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    scroll(event->pos().x()-lastDragPos.x(), event->pos().y()-lastDragPos.y());
    lastDragPos = event->pos();
    update();
  }
}

void MapBox::fitInWindow(){

  QPixmap pixmap = QPixmap::fromImage(currentLayer);
  double tempXScale = ( (double) this->width()) / ((double)pixmap.width());
  double tempYScale = ( (double) this->height())/ ((double)pixmap.height());

  sx1 = 0;
  sy1 = 0;
  sx2 = pixmap.width();
  sy2 = pixmap.height();
  dx1 = 0;
  dy1 = 0;

  if ( tempXScale >= tempYScale){
    dx2 = ((double)pixmap.width())*tempYScale;
    dy2 = this->height();
  } else {
    dx2 = this->width();
    dy2 = ((double)pixmap.height())*tempXScale;
  }
  update();
}

void
MapBox::zoom(double zoomFactor){
  double dx1new, dx2new, dy1new, dy2new;
  dx1new = dx2- (dx2-dx1)* ( 1+ (zoomFactor-1)/2 ); 
  dx2new = dx1+ (dx2-dx1)* ( 1+ (zoomFactor-1)/2 );
  dy1new = dy2- (dy2-dy1)* ( 1+ (zoomFactor-1)/2 ); 
  dy2new = dy1+ (dy2-dy1)* ( 1+ (zoomFactor-1)/2 );
  dx1 = dx1new;
  dy1 = dy1new;
  dx2 = dx2new;
  dy2 = dy2new;
  update();
}

void
MapBox::scroll(int deltaX, int deltaY){
  dx1 += ((double) deltaX);
  dx2 += ((double) deltaX);
  dy1 += ((double) deltaY);
  dy2 += ((double) deltaY);
  update();
}

