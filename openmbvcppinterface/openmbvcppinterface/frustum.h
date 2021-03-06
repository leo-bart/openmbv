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

#ifndef _OPENMBV_FRUSTUM_H_
#define _OPENMBV_FRUSTUM_H_

#include <openmbvcppinterface/rigidbody.h>

namespace OpenMBV {

  /** A frustum (with a frustum hole) */
  class Frustum : public RigidBody {
    friend class ObjectFactory;
    protected:
      double baseRadius{1};
      double topRadius{1};
      double height{2};
      double innerBaseRadius{0};
      double innerTopRadius{0};
      Frustum();
      ~Frustum() override = default;
    public:
      /** Set the radius of the outer side at the base (bottom) */
      void setBaseRadius(double radius) {
        baseRadius=radius;
      } 

      double getBaseRadius() { return baseRadius; }

      /** Set the radius of the outer side at the top. */
      void setTopRadius(double radius) {
        topRadius=radius;
      } 

      double getTopRadius() { return topRadius; }

      /** Set height of the frustum */
      void setHeight(double height_) {
        height=height_;
      } 

      double getHeight() { return height; }

      /** Set the radius of the inner side at the base (bottom). */
      void setInnerBaseRadius(double radius) {
        innerBaseRadius=radius;
      } 

      double getInnerBaseRadius() { return innerBaseRadius; }

      /** Set the radius of the inner side at the top. */
      void setInnerTopRadius(double radius) {
        innerTopRadius=radius;
      } 

      double getInnerTopRadius() { return innerTopRadius; }

      /** Initializes the time invariant part of the object using a XML node */
      void initializeUsingXML(xercesc::DOMElement *element) override;

      xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *parent) override;
  };

}

#endif
