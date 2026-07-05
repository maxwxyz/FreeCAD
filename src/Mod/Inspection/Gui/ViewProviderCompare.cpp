// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 The FreeCAD project association AISBL              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>

#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/PropertyTopoShape.h>

#include "ViewProviderCompare.h"


using namespace InspectionGui;

PROPERTY_SOURCE(InspectionGui::ViewProviderCompare, Gui::ViewProviderDocumentObject)

ViewProviderCompare::ViewProviderCompare()
    : root(new SoSeparator)
{
    root->ref();
    SelectionStyle.setValue(1);
}

ViewProviderCompare::~ViewProviderCompare()
{
    root->unref();
}

void ViewProviderCompare::attach(App::DocumentObject* object)
{
    inherited::attach(object);
    addDisplayMaskMode(root, "Compare");
    rebuild();
}

void ViewProviderCompare::updateData(const App::Property* property)
{
    if (property->is<Part::PropertyPartShape>()) {
        rebuild();
        return;
    }

    inherited::updateData(property);
}

void ViewProviderCompare::setDisplayMode(const char* modeName)
{
    if (strcmp(modeName, "Compare") == 0) {
        setDisplayMaskMode("Compare");
    }

    inherited::setDisplayMode(modeName);
}

std::vector<std::string> ViewProviderCompare::getDisplayModes() const
{
    return {"Compare"};
}

QIcon ViewProviderCompare::getIcon() const
{
    return Gui::BitmapFactory().pixmap("inspection_compare");
}

void ViewProviderCompare::rebuild()
{
    Gui::coinRemoveAllChildren(root);

    if (!pcObject) {
        return;
    }

    auto* common = pcObject->getPropertyByName<Part::PropertyPartShape>("CommonShape");
    auto* added = pcObject->getPropertyByName<Part::PropertyPartShape>("AddedShape");
    auto* removed = pcObject->getPropertyByName<Part::PropertyPartShape>("RemovedShape");

    addShape(common, 0.10F, 0.33F, 0.95F, 0.55F);
    addShape(added, 0.00F, 0.72F, 0.26F, 0.10F);
    addShape(removed, 0.90F, 0.08F, 0.06F, 0.10F);
}

void ViewProviderCompare::addShape(
    const Part::PropertyPartShape* property,
    float r,
    float g,
    float b,
    float transparency
)
{
    if (!property || property->getValue().IsNull()) {
        return;
    }

    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Facet> faces;
    const Data::ComplexGeoData* data = property->getComplexData();
    if (!data) {
        return;
    }

    data->getFaces(points, faces, data->getAccuracy());
    if (points.empty() || faces.empty()) {
        return;
    }

    auto* separator = new SoSeparator;

    auto* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    separator->addChild(hints);

    auto* material = new SoMaterial;
    material->diffuseColor.setValue(r, g, b);
    material->transparency.setValue(transparency);
    separator->addChild(material);

    auto* coords = new SoCoordinate3;
    coords->point.setNum(static_cast<int>(points.size()));
    SbVec3f* coordValues = coords->point.startEditing();
    for (std::size_t index = 0; index < points.size(); ++index) {
        const Base::Vector3d& point = points[index];
        coordValues[index].setValue(
            static_cast<float>(point.x),
            static_cast<float>(point.y),
            static_cast<float>(point.z)
        );
    }
    coords->point.finishEditing();
    separator->addChild(coords);

    auto* faceSet = new SoIndexedFaceSet;
    faceSet->coordIndex.setNum(static_cast<int>(faces.size() * 4));
    int32_t* indices = faceSet->coordIndex.startEditing();
    for (std::size_t index = 0; index < faces.size(); ++index) {
        const Data::ComplexGeoData::Facet& face = faces[index];
        indices[4 * index + 0] = face.I1;
        indices[4 * index + 1] = face.I2;
        indices[4 * index + 2] = face.I3;
        indices[4 * index + 3] = SO_END_FACE_INDEX;
    }
    faceSet->coordIndex.finishEditing();
    separator->addChild(faceSet);

    root->addChild(separator);
}
