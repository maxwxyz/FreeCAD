# ***************************************************************************
# *   Copyright (c) 2016 Ofentse Kgoa <kgoaot@eskom.co.za>                  *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Based on the FemElementGeometry1D by Bernd Hahnebach                  *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM _element fluid 1D document object"
__author__ = "Ofentse Kgoa"
__url__ = "https://www.freecad.org"

## @package element_fluid1D
#  \ingroup FEM
#  \brief element fluid 1D object

from . import base_femelement


class ElementFluid1D(base_femelement.BaseFemElement):
    """
    The element_fluid1D object
    """

    Type = "Fem::ElementFluid1D"

    known_fluid_types = ["Liquid"]
    # "Gas", "Open Channel" are not implemented in ccx writer
    # known_fluid_types = ["Liquid", "Gas", "Open Channel"]
    known_liquid_types = [
        "PIPE MANNING",
        "PIPE ENLARGEMENT",
        "PIPE CONTRACTION",
        "PIPE INLET",
        "PIPE OUTLET",
        "PIPE ENTRANCE",
        "PIPE DIAPHRAGM",
        "PIPE BEND",
        "PIPE GATE VALVE",
        "LIQUID PUMP",
        "PIPE WHITE-COLEBROOK",
    ]
    known_gas_types = ["NONE"]
    known_channel_types = ["NONE"]

    def __init__(self, obj):
        super().__init__(obj)

        obj.addProperty(
            "App::PropertyEnumeration", "SectionType", "FluidSection", "select fluid section type"
        )
        obj.setPropertyStatus("SectionType", "LockDynamic")

        obj.addProperty(
            "App::PropertyEnumeration",
            "LiquidSectionType",
            "LiquidSection",
            "select liquid section type",
        )
        obj.setPropertyStatus("LiquidSectionType", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "ManningArea",
            "LiquidManning",
            "set area of the manning fluid section",
        )
        obj.setPropertyStatus("ManningArea", "LockDynamic")

        obj.addProperty(
            "App::PropertyLength",
            "ManningRadius",
            "LiquidManning",
            "set hydraulic radius of manning fluid section",
        )
        obj.setPropertyStatus("ManningRadius", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "ManningCoefficient",
            "LiquidManning",
            "set coefficient of manning fluid section",
        )
        obj.setPropertyStatus("ManningCoefficient", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "EnlargeArea1",
            "LiquidEnlargement",
            "set initial area of the enlargement fluid section",
        )
        obj.setPropertyStatus("EnlargeArea1", "LockDynamic")
        obj.addProperty(
            "App::PropertyArea",
            "EnlargeArea2",
            "LiquidEnlargement",
            "set enlarged area of enlargement fluid section",
        )
        obj.setPropertyStatus("EnlargeArea2", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "ContractArea1",
            "LiquidContraction",
            "set initial area of the contraction fluid section",
        )
        obj.setPropertyStatus("ContractArea1", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "ContractArea2",
            "LiquidContraction",
            "set contracted area of contraction fluid section",
        )
        obj.setPropertyStatus("ContractArea2", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "InletPressure",
            "LiquidInlet",
            "set inlet pressure for fluid section",
        )
        obj.setPropertyStatus("InletPressure", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "OutletPressure",
            "LiquidOutlet",
            "set outlet pressure for fluid section",
        )
        obj.setPropertyStatus("OutletPressure", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "InletFlowRate",
            "LiquidInlet",
            "set inlet mass flow rate for fluid section",
        )
        obj.setPropertyStatus("InletFlowRate", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "OutletFlowRate",
            "LiquidOutlet",
            "set outlet mass flow rate for fluid section",
        )
        obj.setPropertyStatus("OutletFlowRate", "LockDynamic")

        obj.addProperty(
            "App::PropertyBool",
            "InletPressureActive",
            "LiquidInlet",
            "activates or deactivates inlet pressure for fluid section",
        )
        obj.setPropertyStatus("InletPressureActive", "LockDynamic")

        obj.addProperty(
            "App::PropertyBool",
            "OutletPressureActive",
            "LiquidOutlet",
            "activates or deactivates outlet pressure for fluid section",
        )
        obj.setPropertyStatus("OutletPressureActive", "LockDynamic")

        obj.addProperty(
            "App::PropertyBool",
            "InletFlowRateActive",
            "LiquidInlet",
            "activates or deactivates inlet flow rate for fluid section",
        )
        obj.setPropertyStatus("InletFlowRateActive", "LockDynamic")

        obj.addProperty(
            "App::PropertyBool",
            "OutletFlowRateActive",
            "LiquidOutlet",
            "activates or deactivates outlet flow rate for fluid section",
        )
        obj.setPropertyStatus("OutletFlowRateActive", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "EntrancePipeArea",
            "LiquidEntrance",
            "set the pipe area of the entrance fluid section",
        )
        obj.setPropertyStatus("EntrancePipeArea", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "EntranceArea",
            "LiquidEntrance",
            "set the entrance area of the entrance fluid section",
        )
        obj.setPropertyStatus("EntranceArea", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "DiaphragmPipeArea",
            "LiquidDiaphragm",
            "set the pipe area of the diaphragm fluid section",
        )
        obj.setPropertyStatus("DiaphragmPipeArea", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "DiaphragmArea",
            "LiquidDiaphragm",
            "set the diaphragm area of the diaphragm fluid section",
        )
        obj.setPropertyStatus("DiaphragmArea", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "BendPipeArea",
            "LiquidBend",
            "set pipe area of the bend fluid section",
        )
        obj.setPropertyStatus("BendPipeArea", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "BendRadiusDiameter",
            "LiquidBend",
            "set ratio of bend radius over pipe diameter of the bend fluid section",
        )
        obj.setPropertyStatus("BendRadiusDiameter", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "BendAngle",
            "LiquidBend",
            "set bend angle of the bend fluid section",
        )
        obj.setPropertyStatus("BendAngle", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "BendLossCoefficient",
            "LiquidBend",
            "set loss coefficient of the bend fluid section",
        )
        obj.setPropertyStatus("BendLossCoefficient", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "GateValvePipeArea",
            "LiquidGateValve",
            "set pipe area of the gate valve fluid section",
        )
        obj.setPropertyStatus("GateValvePipeArea", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "GateValveClosingCoeff",
            "LiquidGateValve",
            "set closing coefficient of the gate valve fluid section",
        )
        obj.setPropertyStatus("GateValveClosingCoeff", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloatList",
            "PumpFlowRate",
            "LiquidPump",
            "set the pump characteristic flow rate of the pump fluid section",
        )
        obj.setPropertyStatus("PumpFlowRate", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloatList",
            "PumpHeadLoss",
            "LiquidPump",
            "set the pump characteristic head loss of the pump fluid section",
        )
        obj.setPropertyStatus("PumpHeadLoss", "LockDynamic")

        obj.addProperty(
            "App::PropertyArea",
            "ColebrookeArea",
            "LiquidColebrooke",
            "set pipe area of the colebrooke fluid section",
        )
        obj.setPropertyStatus("ColebrookeArea", "LockDynamic")

        obj.addProperty(
            "App::PropertyLength",
            "ColebrookeRadius",
            "LiquidColebrooke",
            "set hydraulic radius of the colebrooke fluid section",
        )
        obj.setPropertyStatus("ColebrookeRadius", "LockDynamic")

        obj.addProperty(
            "App::PropertyLength",
            "ColebrookeGrainDiameter",
            "LiquidColebrooke",
            "set grain diameter of the colebrooke fluid section",
        )
        obj.setPropertyStatus("ColebrookeGrainDiameter", "LockDynamic")

        obj.addProperty(
            "App::PropertyFloat",
            "ColebrookeFormFactor",
            "LiquidColebrooke",
            "set coefficient of the colebrooke fluid section",
        )
        obj.setPropertyStatus("ColebrookeFormFactor", "LockDynamic")

        obj.addProperty(
            "App::PropertyEnumeration", "GasSectionType", "GasSection", "select gas section type"
        )
        obj.setPropertyStatus("GasSectionType", "LockDynamic")

        obj.addProperty(
            "App::PropertyEnumeration",
            "ChannelSectionType",
            "ChannelSection",
            "select channel section type",
        )
        obj.setPropertyStatus("ChannelSectionType", "LockDynamic")

        # set property default values
        obj.SectionType = ElementFluid1D.known_fluid_types
        obj.SectionType = "Liquid"
        obj.LiquidSectionType = ElementFluid1D.known_liquid_types
        obj.LiquidSectionType = "PIPE INLET"
        obj.GasSectionType = ElementFluid1D.known_gas_types
        obj.GasSectionType = "NONE"
        obj.ChannelSectionType = ElementFluid1D.known_channel_types
        obj.ChannelSectionType = "NONE"
        obj.ManningArea = 10.0
        obj.ManningRadius = 1.0
        obj.ManningCoefficient = 0.0015  # has units of s/mm^(1/3)
        obj.EnlargeArea1 = 10.0
        obj.EnlargeArea2 = 20.0
        obj.ContractArea1 = 20.0
        obj.ContractArea2 = 10.0
        obj.EntrancePipeArea = 20.0
        obj.EntranceArea = 20.0
        obj.DiaphragmPipeArea = 20.0
        obj.DiaphragmArea = 20.0
        obj.BendPipeArea = 20.0
        obj.BendRadiusDiameter = 1.0
        obj.BendAngle = 0.0
        obj.BendLossCoefficient = 0.0
        obj.GateValvePipeArea = 20.0
        obj.GateValveClosingCoeff = 0.125
        obj.PumpFlowRate = [0, 1.04e-04, 2.08e-4, 3.13e-4, 4.17e-4]
        obj.PumpHeadLoss = [30, 29.17, 26.67, 23.33, 18.33]
        obj.ColebrookeArea = 20.0
        obj.ColebrookeRadius = 1.0
        obj.ColebrookeGrainDiameter = 0.0025
        obj.ColebrookeFormFactor = 1.0
        obj.InletPressure = 1.0
        obj.OutletPressure = 1.0
        obj.InletFlowRate = 1.0
        obj.OutletFlowRate = 1.0
        obj.InletPressureActive = True
        obj.OutletPressureActive = True
        obj.InletFlowRateActive = False
        obj.OutletFlowRateActive = False
