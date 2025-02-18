/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Gus Grubba <mavlink@grubba.com>

#include "ScreenToolsController.h"
#include "MainWindow.h"

int ScreenToolsController::_qmlDefaultFontPixelSize = -1;

#ifdef Q_OS_WIN
const double ScreenToolsController::_defaultFontPixelSizeRatio = 1.0;
#else
const double ScreenToolsController::_defaultFontPixelSizeRatio = 0.8;
#endif
const double ScreenToolsController::_smallFontPixelSizeRatio = 0.75;
const double ScreenToolsController::_mediumFontPixelSizeRatio = 1.22;
const double ScreenToolsController::_largeFontPixelSizeRatio =  1.66;

ScreenToolsController::ScreenToolsController()
{
    MainWindow* mainWindow = MainWindow::instance();
    // Unit tests can run Qml without MainWindow
    if (mainWindow) {
        connect(mainWindow, &MainWindow::repaintCanvas, this, &ScreenToolsController::_updateCanvas);
    }
}

void ScreenToolsController::_updateCanvas()
{
    emit repaintRequested();
}

double ScreenToolsController::getQmlDefaultFontPixelSize(void)
{
    if (_qmlDefaultFontPixelSize == -1) {
        QGCQmlWidgetHolder qmlWidgetHolder;
        
        qmlWidgetHolder.setSource(QUrl::fromUserInput("qrc:/qml/ScreenToolsFontQuery.qml"));
    }

    double qmlDefaultFontPixelSize = _qmlDefaultFontPixelSize;
    
#ifdef QT_DEBUG
    if (qgcApp()->testHighDPI()) {
        qmlDefaultFontPixelSize *= 2;
    }
#endif
    
    return qmlDefaultFontPixelSize;
}
