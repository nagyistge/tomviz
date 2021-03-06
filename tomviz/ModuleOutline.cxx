/******************************************************************************
  This source file is part of the tomviz project.

  Copyright Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/
#include "ModuleOutline.h"

#include "DataSource.h"
#include "Utilities.h"
#include "pqProxiesWidget.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include <pqColorChooserButton.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace tomviz {

ModuleOutline::ModuleOutline(QObject* parentObject) : Superclass(parentObject)
{
}

ModuleOutline::~ModuleOutline()
{
  this->finalize();
}

QIcon ModuleOutline::icon() const
{
  return QIcon(":/pqWidgets/Icons/pqProbeLocation24.png");
}

bool ModuleOutline::initialize(DataSource* data, vtkSMViewProxy* vtkView)
{
  if (!this->Superclass::initialize(data, vtkView)) {
    return false;
  }

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  vtkSMSessionProxyManager* pxm = data->producer()->GetSessionProxyManager();

  // Create the outline filter.
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("filters", "OutlineFilter"));

  this->OutlineFilter = vtkSMSourceProxy::SafeDownCast(proxy);
  Q_ASSERT(this->OutlineFilter);
  controller->PreInitializeProxy(this->OutlineFilter);
  vtkSMPropertyHelper(this->OutlineFilter, "Input").Set(data->producer());
  controller->PostInitializeProxy(this->OutlineFilter);
  controller->RegisterPipelineProxy(this->OutlineFilter);

  // Create the representation for it.
  this->OutlineRepresentation =
    controller->Show(this->OutlineFilter, 0, vtkView);
  vtkSMPropertyHelper(this->OutlineRepresentation, "Position")
    .Set(data->displayPosition(), 3);
  vtkSMPropertyHelper(this->OutlineRepresentation, "DiffuseColor")
    .Set(offWhite, 3);
  Q_ASSERT(this->OutlineRepresentation);
  // vtkSMPropertyHelper(this->OutlineRepresentation,
  //                    "Representation").Set("Outline");
  this->OutlineRepresentation->UpdateVTKObjects();
  return true;
}

bool ModuleOutline::finalize()
{
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  controller->UnRegisterProxy(this->OutlineRepresentation);
  controller->UnRegisterProxy(this->OutlineFilter);

  this->OutlineFilter = nullptr;
  this->OutlineRepresentation = nullptr;
  return true;
}

bool ModuleOutline::serialize(pugi::xml_node& ns) const
{
  // save stuff that the user can change.
  pugi::xml_node reprNode = ns.append_child("OutlineRepresentation");

  QStringList properties;
  properties << "Visibility"
             << "DiffuseColor";
  if (tomviz::serialize(this->OutlineRepresentation, reprNode, properties) ==
      false) {
    qWarning("Failed to serialize ModuleOutline.");
    ns.remove_child(reprNode);
    return false;
  }
  return true;
}

bool ModuleOutline::deserialize(const pugi::xml_node& ns)
{
  return tomviz::deserialize(this->OutlineRepresentation,
                             ns.child("OutlineRepresentation"));
}

bool ModuleOutline::setVisibility(bool val)
{
  Q_ASSERT(this->OutlineRepresentation);
  vtkSMPropertyHelper(this->OutlineRepresentation, "Visibility")
    .Set(val ? 1 : 0);
  this->OutlineRepresentation->UpdateVTKObjects();
  return true;
}

bool ModuleOutline::visibility() const
{
  Q_ASSERT(this->OutlineRepresentation);
  return vtkSMPropertyHelper(this->OutlineRepresentation, "Visibility")
           .GetAsInt() != 0;
}

void ModuleOutline::addToPanel(QWidget* panel)
{
  Q_ASSERT(panel && this->OutlineRepresentation);

  if (panel->layout()) {
    delete panel->layout();
  }

  QHBoxLayout* layout = new QHBoxLayout;
  QLabel* label = new QLabel("Color");
  layout->addWidget(label);
  layout->addStretch();
  pqColorChooserButton* colorSelector = new pqColorChooserButton(panel);
  colorSelector->setShowAlphaChannel(false);
  layout->addWidget(colorSelector);

  QVBoxLayout* panelLayout = new QVBoxLayout;
  panelLayout->addItem(layout);
  panelLayout->addStretch();
  panel->setLayout(panelLayout);

  m_links.addPropertyLink(
    colorSelector, "chosenColorRgbF", SIGNAL(chosenColorChanged(const QColor&)),
    this->OutlineRepresentation,
    this->OutlineRepresentation->GetProperty("DiffuseColor"));

  this->connect(colorSelector, &pqColorChooserButton::chosenColorChanged, this,
                &ModuleOutline::dataUpdated);
}

void ModuleOutline::dataUpdated()
{
  m_links.accept();
  emit this->renderNeeded();
}

void ModuleOutline::dataSourceMoved(double newX, double newY, double newZ)
{
  double pos[3] = { newX, newY, newZ };
  vtkSMPropertyHelper(this->OutlineRepresentation, "Position").Set(pos, 3);
  this->OutlineRepresentation->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool ModuleOutline::isProxyPartOfModule(vtkSMProxy* proxy)
{
  return (proxy == this->OutlineFilter.Get()) ||
         (proxy == this->OutlineRepresentation.Get());
}

std::string ModuleOutline::getStringForProxy(vtkSMProxy* proxy)
{
  if (proxy == this->OutlineFilter.Get()) {
    return "Outline";
  } else if (proxy == this->OutlineRepresentation.Get()) {
    return "Representation";
  } else {
    qWarning("Unknown proxy passed to module outline in save animation");
    return "";
  }
}

vtkSMProxy* ModuleOutline::getProxyForString(const std::string& str)
{
  if (str == "Outline") {
    return this->OutlineFilter.Get();
  } else if (str == "Representation") {
    return this->OutlineRepresentation.Get();
  } else {
    return nullptr;
  }
}

} // end of namespace tomviz
