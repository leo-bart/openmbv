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

#ifndef _OPENMBVGUI_VIBODYCUBE_H_
#define _OPENMBVGUI_VIBODYCUBE_H_

#include "rigidbody.h"
#include <string>
#include <QThread>
#include <openmbvcppinterface/ivbody.h>

namespace OpenMBV {
  class IvBody;
}

namespace OpenMBVGUI {

class EdgeCalculation;

class IvBody : public RigidBody {
  Q_OBJECT
  public:
    IvBody(const std::shared_ptr<OpenMBV::Object> &obj, QTreeWidgetItem *parentItem, SoGroup *soParent, int ind);
    ~IvBody() override;
  protected:
    std::shared_ptr<OpenMBV::IvBody> ivb;
    void createProperties() override;

  private:
    EdgeCalculation *edgeCalc;
    void calculateEdges(const std::string& fullName, double creaseEdges, bool boundaryEdges);
    class CalculateEdgesThread : public QThread {
      public:
        CalculateEdgesThread(IvBody *ivBody_) : ivBody(ivBody_) {}
      protected:
        void run() override {
          std::shared_ptr<OpenMBV::IvBody> ivb=std::static_pointer_cast<OpenMBV::IvBody>(ivBody->object);
          ivBody->calculateEdges(ivb->getFullName(), ivb->getCreaseEdges(), ivb->getBoundaryEdges());
        }
        IvBody *ivBody;
    };
    CalculateEdgesThread calculateEdgesThread;
  private:
    void addEdgesToScene();
  Q_SIGNALS:
    void statusBarShowMessage(const QString &message, int timeout=0);
};

}

#endif
