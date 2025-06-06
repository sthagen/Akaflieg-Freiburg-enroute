/***************************************************************************
 *   Copyright (C) 2021-2024 by Stefan Kebekus                             *
 *   stefan.kebekus@gmail.com                                              *
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

#include "positioning/PositionInfoSource_Abstract.h"


Positioning::PositionInfoSource_Abstract::PositionInfoSource_Abstract(QObject* parent) : QObject(parent)
{
    // Setup timer
    m_positionInfoTimer.setInterval( PositionInfo::lifetime );
    m_positionInfoTimer.setSingleShot(true);
    connect(&m_positionInfoTimer, &QTimer::timeout, this, [this]() {setPositionInfo({});});
}


void Positioning::PositionInfoSource_Abstract::setPositionInfo(const Positioning::PositionInfo& info)
{
    if (info.isValid())
    {
        m_positionInfoTimer.start();
    }

    Qt::beginPropertyUpdateGroup();
    m_positionInfo = info;
    m_receivingPositionInfo = info.isValid();
    Qt::endPropertyUpdateGroup();
}
