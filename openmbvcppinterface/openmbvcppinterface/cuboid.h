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

#ifndef _OPENMBV_CUBOID_H_
#define _OPENMBV_CUBOID_H_

#include <openmbvcppinterface/rigidbody.h>

namespace OpenMBV {

  /** A cuboid */
  class Cuboid : public RigidBody {
    protected:
      VectorParameter length;
      ~Cuboid() {}
    public:
      /** Default constructor */
      Cuboid();

      /** Retrun the class name */
      std::string getClassName() { return "Cuboid"; }

      /** Set the length of the cuboid */
      void setLength(const VectorParameter& length_) {
        assert(length_.getParamStr()!="" || length_.getValue().size()==3);
        set(length,length_);
      } 

      /** Set the length of the cuboid */
      void setLength(const std::vector<double>& length_) {
        assert(length_.size()==3);
        set(length,length_);
      } 

      std::vector<double> getLength() { return get(length); }

      /** Set the length of the cuboid */
      void setLength(double x, double y, double z) {
        std::vector<double> length_;
        length_.push_back(x);
        length_.push_back(y);
        length_.push_back(z);
        length=length_;
      } 

      /** Initializes the time invariant part of the object using a XML node */
      virtual void initializeUsingXML(TiXmlElement *element);

      TiXmlElement* writeXMLFile(TiXmlNode *parent);
  };

}

#endif