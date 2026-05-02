/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#endif  // _PreComp_

#include <Base/Console.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>
#include <Base/QuantityPy.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>

#include "PropertyDressUp.h"

using namespace Part;

TYPESYSTEM_SOURCE(Part::PropertyFilletSegments, App::Property)

PropertyFilletSegments::PropertyFilletSegments()
{}

PropertyFilletSegments::~PropertyFilletSegments()
{}

void PropertyFilletSegments::setValue(const std::string& id, double param, double radius, double length)
{
    auto it = segmentsMap.find(id);
    if (param >= 0.0 && param <= 1.0 && it == segmentsMap.end()) {
        return;
    }
    if (length > 0.0) {
        for (auto& segment : it->second) {
            if (segment.length == length) {
                aboutToSetValue();
                segment.param = 0.0;
                segment.radius = radius;
                hasSetValue();
                return;
            }
        }
    }
    else {
        for (auto& segment : it->second) {
            if (segment.param == param) {
                aboutToSetValue();
                segment.radius = radius;
                segment.length = 0.0;
                hasSetValue();
                return;
            }
        }
    }
    aboutToSetValue();
    it->second.emplace_back(param, radius);
    hasSetValue();
}

void PropertyFilletSegments::setValue(const std::string& id, Segments&& segments)
{
    aboutToSetValue();
    segmentsMap[id] = std::move(segments);
    hasSetValue();
}

void PropertyFilletSegments::removeValue(const std::string& id, double param, double length)
{
    auto it = segmentsMap.find(id);
    if (it == segmentsMap.end()) {
        return;
    }
    AtomicPropertyChange signaller(*this, false);
    if (length > 0.0) {
        for (auto iter = it->second.begin(); iter != it->second.end(); ++iter) {
            if (iter->length == length) {
                signaller.aboutToChange();
                it->second.erase(iter);
                break;
            }
        }
    }
    else {
        for (auto iter = it->second.begin(); iter != it->second.end(); ++iter) {
            if (iter->param == param) {
                signaller.aboutToChange();
                it->second.erase(iter);
                break;
            }
        }
    }
    signaller.tryInvoke();
}

void PropertyFilletSegments::removeValue(const std::string& id, int index)
{
    auto it = segmentsMap.find(id);
    if (it == segmentsMap.end() || index < 0 || index >= static_cast<int>(it->second.size())) {
        return;
    }
    aboutToSetValue();
    it->second.erase(it->second.begin() + index);
    hasSetValue();
}

void PropertyFilletSegments::removeValue(const std::string& id)
{
    auto it = segmentsMap.find(id);
    if (it == segmentsMap.end()) {
        return;
    }
    aboutToSetValue();
    segmentsMap.erase(it);
    hasSetValue();
}

void PropertyFilletSegments::setValue(std::map<std::string, Segments>&& values)
{
    aboutToSetValue();
    segmentsMap = std::move(values);
    hasSetValue();
}

void PropertyFilletSegments::setValue(const std::map<std::string, Segments>& values)
{
    aboutToSetValue();
    segmentsMap = values;
    hasSetValue();
}

const std::map<std::string, PropertyFilletSegments::Segments>& PropertyFilletSegments::getValue() const
{
    return segmentsMap;
}

const PropertyFilletSegments::Segments& PropertyFilletSegments::getValue(const std::string& sub) const
{
    static const Segments nullSegment;
    auto it = segmentsMap.find(sub);
    if (it == segmentsMap.end()) {
        return nullSegment;
    }
    return it->second;
}

void PropertyFilletSegments::connectLinkProperty(App::PropertyLinkSub& links)
{
    connChanged = links.signalChanged.connect([this](const App::Property& prop) {
        std::map<std::string, Segments> value;
        auto obj = dynamic_cast<App::DocumentObject*>(getContainer());
        std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;
        for (const auto& sub : static_cast<const App::PropertyLinkSub&>(prop).getSubValues(false)) {
            auto it = referenceUpdates.find(sub);
            const auto& key = (it == referenceUpdates.end()) ? sub : it->second;
            auto iter = segmentsMap.find(key);
            if (iter == segmentsMap.end()) {
                continue;
            }
            if (obj) {
                App::ObjectIdentifier path(*this);
                path << App::ObjectIdentifier::SimpleComponent(sub);
                App::ObjectIdentifier pathNew(*this);
                pathNew << App::ObjectIdentifier::SimpleComponent(key);
                renames.emplace(path, pathNew);
                for (int i = 0; i < static_cast<int>(iter->second.size()); ++i) {
                    App::ObjectIdentifier p1(path);
                    p1 << App::ObjectIdentifier::ArrayComponent(i);
                    App::ObjectIdentifier p2(pathNew);
                    p2 << App::ObjectIdentifier::ArrayComponent(i);
                    renames.emplace(p1, p2);
                    renames.emplace(
                        App::ObjectIdentifier(p1) << App::ObjectIdentifier::SimpleComponent("Radius"),
                        App::ObjectIdentifier(p2) << App::ObjectIdentifier::SimpleComponent("Radius")
                    );
                    renames.emplace(
                        App::ObjectIdentifier(p1) << App::ObjectIdentifier::SimpleComponent("Param"),
                        App::ObjectIdentifier(p2) << App::ObjectIdentifier::SimpleComponent("Param")
                    );
                    renames.emplace(
                        App::ObjectIdentifier(p1) << App::ObjectIdentifier::SimpleComponent("Length"),
                        App::ObjectIdentifier(p2) << App::ObjectIdentifier::SimpleComponent("Length")
                    );
                }
            }
            value[sub] = iter->second;
        }
        if (value != segmentsMap) {
            setValue(std::move(value));
        }
        if (obj) {
            obj->ExpressionEngine.renameExpressions(renames);
            for (auto doc : App::GetApplication().getDocuments()) {
                doc->renameObjectIdentifiers(renames);
            }
        }
        referenceUpdates.clear();
    });

    connUpdateReference = links.signalUpdateElementReference.connect(
        [this](const std::string& sub, const std::string& newSub) {
            referenceUpdates.emplace(newSub, sub);
        }
    );
}

PyObject* PropertyFilletSegments::getPyObject()
{
    Py::List list(segmentsMap.size());
    int i = 0;
    for (auto& v : segmentsMap) {
        Py::List entry(v.second.size());
        int j = 0;
        for (auto& segment : v.second) {
            entry[j++] = Py::TupleN(
                Py::Float(segment.param),
                Py::Float(segment.radius),
                Py::Float(segment.length)
            );
        }
        list[i++] = entry;
    }
    return Py::new_reference_to(list);
}

void PropertyFilletSegments::setPyObject(PyObject* pyobj)
{
    const char* msg
        = "Expect the input to be type of "
          "Sequence(Tuple(element:String, Sequence(param:Float, radius:Float, length:Float)))";
    try {
        std::map<std::string, Segments> value;
        Py::Sequence seq(pyobj);
        for (int i = 0; i < seq.size(); ++i) {
            Py::Sequence item(seq[i].ptr());
            if (item.size() != 2) {
                throw Base::TypeError(msg);
            }
            auto& segments = value[Py::String(item[0].ptr())];
            Py::Sequence pySegments(Py::Sequence(item[1].ptr()));
            for (int j = 0; j < pySegments.size(); ++j) {
                Py::Sequence pySegment(pySegments[j].ptr());
                if (pySegment.size() < 2 || pySegment.size() > 3) {
                    throw Base::TypeError(msg);
                }
                segments.emplace_back(
                    Py::Float(pySegment[0].ptr()),
                    Py::Float(pySegment[1].ptr()),
                    pySegment.size() == 2 ? 0.0 : Py::Float(pySegment[2].ptr())
                );
            }
        }
        if (value != segmentsMap) {
            setValue(std::move(value));
        }
    }
    catch (Py::Exception&) {
        throw Base::TypeError(msg);
    }
}

void PropertyFilletSegments::Save(Base::Writer& writer) const
{
    int count = 0;
    for (const auto& v : segmentsMap) {
        if (!v.second.empty()) {
            ++count;
        }
    }
    if (count == 0) {
        writer.Stream() << writer.ind() << "<EdgeSegments/>\n";
        return;
    }

    writer.Stream() << writer.ind() << "<EdgeSegments count=\"" << count << "\">\n";
    writer.incInd();
    for (const auto& v : segmentsMap) {
        if (v.second.empty()) {
            continue;
        }
        writer.Stream() << writer.ind() << "<Segments id=\"" << encodeAttribute(v.first)
                        << "\" count=\"" << v.second.size() << "\">\n";
        writer.incInd();
        for (const auto& segment : v.second) {
            writer.Stream() << writer.ind() << "<Segment param=\"" << segment.param << "\" radius=\""
                            << segment.radius << "\" length=\"" << segment.length << "\"/>\n";
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</Segments>\n";
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</EdgeSegments>\n";
}

void PropertyFilletSegments::Restore(Base::XMLReader& reader)
{
    reader.readElement("EdgeSegments");
    long count = reader.getAttribute<long>("count", 0L);
    std::map<std::string, Segments> value;
    for (long i = 0; i < count; ++i) {
        reader.readElement("Segments");
        std::string id = reader.getAttribute<const char*>("id");
        if (!id.empty()) {
            auto& segments = value[id];
            long scount = reader.getAttribute<long>("count", 0L);
            for (long j = 0; j < scount; ++j) {
                reader.readElement("Segment");
                segments.emplace_back(
                    reader.getAttribute<double>("param"),
                    reader.getAttribute<double>("radius"),
                    reader.getAttribute<double>("length", 0.0)
                );
            }
        }
        reader.readEndElement("Segments");
    }
    reader.readEndElement("EdgeSegments");
    setValue(std::move(value));
}

void PropertyFilletSegments::getPaths(std::vector<App::ObjectIdentifier>& paths) const
{
    for (const auto& v : segmentsMap) {
        paths.push_back(
            App::ObjectIdentifier(*this) << App::ObjectIdentifier::SimpleComponent(v.first)
        );
    }
}

bool PropertyFilletSegments::setPyPathValue(const App::ObjectIdentifier& path, const Py::Object& value)
{
    if (path.numSubComponents() < 2 || path.numSubComponents() > 4
        || path.getPropertyComponent(0).getName() != getName()) {
        FC_THROWM(Base::ValueError, "invalid constraint path " << path.toString());
    }

    const App::ObjectIdentifier::Component& c1 = path.getPropertyComponent(1);
    if (!c1.isSimple()) {
        FC_THROWM(Base::ValueError, "invalid path " << path.toString());
    }

    try {
        if (path.numSubComponents() == 2) {
            if (value.isNone()) {
                removeValue(c1.getName());
                return true;
            }
            Segments segments;
            Py::Sequence seq(value);
            for (int i = 0; i < seq.size(); ++i) {
                Py::Sequence pySegment(seq[i].ptr());
                if (pySegment.size() != 2) {
                    FC_THROWM(Base::TypeError, "invalid value");
                }
                segments.emplace_back(Py::Float(pySegment[0]), Py::Float(pySegment[1]));
            }
            setValue(c1.getName(), std::move(segments));
            return true;
        }
        const App::ObjectIdentifier::Component& c2 = path.getPropertyComponent(2);
        if (!c2.isArray()) {
            FC_THROWM(Base::ValueError, "invalid path " << path.toString());
        }
        aboutToSetValue();
        auto& segments = segmentsMap[c1.getName()];
        size_t index = c2.getIndex(segments.size());
        if (path.numSubComponents() > 3) {
            const App::ObjectIdentifier::Component& c3 = path.getPropertyComponent(3);
            if (!c3.isSimple()) {
                FC_THROWM(Base::ValueError, "invalid path " << path.toString());
            }
            if (c3.getName() == "Radius") {
                segments[index].radius = Py::Float(value);
            }
            else if (c3.getName() == "Param") {
                segments[index].param = Py::Float(value);
            }
            else if (c3.getName() == "Length") {
                segments[index].length = Py::Float(value);
            }
            else {
                FC_THROWM(Base::ValueError, "invalid path " << path.toString());
            }
        }
        else {
            Py::Sequence seq(value);
            if (seq.size() == 2) {
                segments[index].param = Py::Float(seq[0]);
                segments[index].radius = Py::Float(seq[1]);
            }
            else if (seq.size() == 3) {
                segments[index].param = Py::Float(seq[0]);
                segments[index].radius = Py::Float(seq[1]);
                segments[index].length = Py::Float(seq[2]);
            }
            else {
                FC_THROWM(Base::TypeError, "invalid value");
            }
        }
        hasSetValue();
    }
    catch (Py::Exception&) {
        FC_THROWM(Base::TypeError, "invalid value");
    }
    return true;
}

bool PropertyFilletSegments::getPyPathValue(const App::ObjectIdentifier& path, Py::Object& res) const
{
    // numSubComponents() counts property name + sub-components.
    // So nsub=2 means "Segments.Edge1", nsub=3 means "Segments.Edge1[0]", etc.
    int nsub = path.numSubComponents();
    int nAfterProp = nsub - 1;  // sub-components after the property name
    if (nAfterProp < 1 || nAfterProp > 3) {
        return false;
    }

    const App::ObjectIdentifier::Component& c1 = path.getPropertyComponent(1);
    if (!c1.isSimple()) {
        return false;
    }
    auto it = segmentsMap.find(c1.getName());
    if (it == segmentsMap.end()) {
        return false;
    }
    const Segments* segments = &it->second;

    if (nAfterProp == 1) {
        Py::List list(segments->size());
        int i = 0;
        for (const auto& segment : *segments) {
            list[i++] = Py::TupleN(
                Py::Float(segment.param),
                Py::asObject(
                    new Base::QuantityPy(new Base::Quantity(segment.radius, Base::Unit::Length))
                ),
                Py::asObject(
                    new Base::QuantityPy(new Base::Quantity(segment.length, Base::Unit::Length))
                )
            );
        }
        res = list;
        return true;
    }

    const App::ObjectIdentifier::Component& c2 = path.getPropertyComponent(2);
    if (!c2.isArray()) {
        return false;
    }
    std::size_t index = c2.getIndex(segments->size());
    const auto& segment = (*segments)[index];

    if (nAfterProp == 2) {
        res = Py::TupleN(
            Py::Float(segment.param),
            Py::asObject(new Base::QuantityPy(new Base::Quantity(segment.radius, Base::Unit::Length))),
            Py::asObject(new Base::QuantityPy(new Base::Quantity(segment.length, Base::Unit::Length)))
        );
        return true;
    }

    const App::ObjectIdentifier::Component& c3 = path.getPropertyComponent(3);
    if (!c3.isSimple()) {
        return false;
    }
    if (c3.getName() == "Radius") {
        res = Py::Float(segment.radius);
    }
    else if (c3.getName() == "Param") {
        res = Py::Float(segment.param);
    }
    else if (c3.getName() == "Length") {
        res = Py::Float(segment.length);
    }
    else {
        return false;
    }
    return true;
}

void PropertyFilletSegments::setPathValue(const App::ObjectIdentifier& path, const boost::any& value)
{
    Base::PyGILStateLocker lock;
    setPyPathValue(path, App::pyObjectFromAny(value));
}

const boost::any PropertyFilletSegments::getPathValue(const App::ObjectIdentifier& path) const
{
    Base::PyGILStateLocker lock;
    Py::Object pyObj;
    if (getPyPathValue(path, pyObj)) {
        return App::pyObjectToAny(pyObj);
    }
    return App::any();
}

App::Property* PropertyFilletSegments::Copy() const
{
    auto* p = new PropertyFilletSegments();
    p->segmentsMap = segmentsMap;
    return p;
}

void PropertyFilletSegments::Paste(const Property& from)
{
    setValue(dynamic_cast<const PropertyFilletSegments&>(from).segmentsMap);
}

unsigned int PropertyFilletSegments::getMemSize() const
{
    unsigned int size = 0;
    for (const auto& v : segmentsMap) {
        size += static_cast<unsigned int>(v.second.size() * sizeof(Segment));
    }
    return size;
}

bool PropertyFilletSegments::isSame(const Property& other) const
{
    if (this == &other) {
        return true;
    }
    if (!other.isDerivedFrom(getClassTypeId())) {
        return false;
    }
    return segmentsMap == static_cast<const PropertyFilletSegments&>(other).segmentsMap;
}
