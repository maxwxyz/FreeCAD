/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef PARTDESIGN_Feature_H
#define PARTDESIGN_Feature_H

#include <App/PropertyStandard.h>
#include <App/SuppressibleExtension.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

class gp_Pnt;
class gp_Pln;


/// Base class of all additive features in PartDesign
namespace PartDesign
{

using TopoShape = Part::TopoShape;

class Body;

 /** PartDesign feature
 *   Base class of all PartDesign features.
 *   This kind of features only produce solids or fail.
 */
class PartDesignExport Feature : public Part::Feature, public App::SuppressibleExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Feature);

public:
    Feature();

    enum SingleSolidRuleMode { Disabled = 0, Enforced = 1 };

    /// Base feature which this feature will be fused into or cut out of
    App::PropertyLink   BaseFeature;
    App::PropertyLinkHidden _Body;

    /// Keep a copy of suppressed shapes so that we can restore them (and maybe display them)
    Part::PropertyPartShape SuppressedShape;
    /// Keep a copy of the placement before suppression to restore it back when unsuppressed, fix #20205
    Base::Placement SuppressedPlacement;
    App::DocumentObjectExecReturn* recompute() override;

    short mustExecute() const override;

    /// Check whether the given feature is a datum feature
    static bool isDatum(const App::DocumentObject* feature);

    /// Returns the body the feature is in, or none
    Body* getFeatureBody() const;

    /**
     * Returns the BaseFeature property's object (if any)
     * @param silent if couldn't determine the base feature and silent == true,
     *               silently return a nullptr, otherwise throw Base::Exception.
     *               Default is false.
     */
    virtual Part::Feature* getBaseObject(bool silent=false) const;
    /// Returns the BaseFeature property's shape (if any)
    virtual const TopoDS_Shape& getBaseShape() const;
    /// Returns the BaseFeature property's TopoShape (if any)
    Part::TopoShape getBaseTopoShape(bool silent=false) const;

    PyObject* getPyObject() override;

    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProvider";
    }

    void onChanged(const App::Property* prop) override;

    App::DocumentObject *getSubObject(const char *subname, 
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const override;


protected:

    /**
     * Get a solid of the given shape. If no solid is found an exception is raised.
     */
    TopoShape getSolid(const TopoShape&) const;
    static int countSolids(const TopoDS_Shape&, TopAbs_ShapeEnum type = TopAbs_SOLID);

    /**
     * Checks if the single-solid body rule is fulfilled.
     */
    bool isSingleSolidRuleSatisfied(const TopoDS_Shape&, TopAbs_ShapeEnum type = TopAbs_SOLID);
    SingleSolidRuleMode singleSolidRuleMode() const;

    void updateSuppressedShape();

    /**
     * Set the Material To Body Material object
     */
    void setMaterialToBodyMaterial();

    /// Grab any point from the given face
    static const gp_Pnt getPointFromFace(const TopoDS_Face& f);
    /// Make a shape from a base plane (convenience method)
    static gp_Pln makePlnFromPlane(const App::DocumentObject* obj);
    // TODO: Toponaming April 2024 Deprecated in favor of TopoShape method.  Remove when possible.
    static TopoDS_Shape makeShapeFromPlane(const App::DocumentObject* obj);
    static TopoShape makeTopoShapeFromPlane(const App::DocumentObject* obj);
};

using FeaturePython = App::FeaturePythonT<Feature>;

} //namespace PartDesign


#endif // PARTDESIGN_Feature_H
