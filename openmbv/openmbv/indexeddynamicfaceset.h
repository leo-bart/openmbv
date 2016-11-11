/*
    OpenMBV - Open Multi Body Viewer.
    Copyright (C) 2009 Markus Friedrich

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _OPENMBVGUI_INDEXEDDYNAMICFACESET_H_
#define _OPENMBVGUI_INDEXEDDYNAMICFACESET_H_

#include "dynamiccoloredbody.h"
#include <string>

namespace OpenMBV {
  class IndexedDynamicFaceSet;
}

namespace OpenMBVGUI {

class IndexedDynamicFaceSet : public DynamicColoredBody {
  Q_OBJECT
  public:
    IndexedDynamicFaceSet(const std::shared_ptr<OpenMBV::Object> &obj, QTreeWidgetItem *parentItem, SoGroup *soParent, int ind);
  protected:
    std::shared_ptr<OpenMBV::IndexedDynamicFaceSet> faceset;
    int numvp;
    SoCoordinate3 *points;
    virtual double update();
    void createProperties();
};

}

#endif
