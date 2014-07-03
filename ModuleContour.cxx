/******************************************************************************

  This source file is part of the TEM tomography project.

  Copyright Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/
#include "ModuleContour.h"

#include "DataSource.h"
#include "pqProxiesWidget.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <vector>
#include <algorithm>

namespace TEM
{

//-----------------------------------------------------------------------------
ModuleContour::ModuleContour(QObject* parentObject) :Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
ModuleContour::~ModuleContour()
{
  this->finalize();
}

//-----------------------------------------------------------------------------
QIcon ModuleContour::icon() const
{
  return QIcon(":/pqWidgets/Icons/pqIsosurface24.png");
}

//-----------------------------------------------------------------------------
bool ModuleContour::initialize(DataSource* dataSource, vtkSMViewProxy* view)
{
  if (!this->Superclass::initialize(dataSource, view))
    {
    return false;
    }

  vtkSMSourceProxy* producer = dataSource->producer();

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  vtkSMSessionProxyManager* pxm = producer->GetSessionProxyManager();

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("filters", "Contour"));

  this->ContourFilter = vtkSMSourceProxy::SafeDownCast(proxy);
  Q_ASSERT(this->ContourFilter);
  controller->PreInitializeProxy(this->ContourFilter);
  vtkSMPropertyHelper(this->ContourFilter, "Input").Set(producer);
  vtkSMPropertyHelper(this->ContourFilter, "ComputeScalars", /*quiet*/ true).Set(1);

  controller->PostInitializeProxy(this->ContourFilter);
  controller->RegisterPipelineProxy(this->ContourFilter);

  // Create the representation for it.
  this->ContourRepresentation = controller->Show(this->ContourFilter, 0, view);
  Q_ASSERT(this->ContourRepresentation);
  vtkSMPropertyHelper(this->ContourRepresentation, "Representation").Set("Surface");
  this->ContourRepresentation->UpdateVTKObjects();
  return true;
}

//-----------------------------------------------------------------------------
bool ModuleContour::finalize()
{
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  controller->UnRegisterProxy(this->ContourRepresentation);
  controller->UnRegisterProxy(this->ContourFilter);
  this->ContourFilter = NULL;
  this->ContourRepresentation = NULL;
  return true;
}

//-----------------------------------------------------------------------------
bool ModuleContour::setVisibility(bool val)
{
  Q_ASSERT(this->ContourRepresentation);
  vtkSMPropertyHelper(this->ContourRepresentation, "Visibility").Set(val? 1 : 0);
  this->ContourRepresentation->UpdateVTKObjects();
  return true;
}

//-----------------------------------------------------------------------------
bool ModuleContour::visibility() const
{
  Q_ASSERT(this->ContourRepresentation);
  return vtkSMPropertyHelper(this->ContourRepresentation, "Visibility").GetAsInt() != 0;
}

//-----------------------------------------------------------------------------
void ModuleContour::setIsoValues(const QList<double>& values)
{
  std::vector<double> vectorValues(values.size());
  std::copy(values.begin(), values.end(), vectorValues.begin());
  vectorValues.push_back(0); // to avoid having to check for 0 size on Windows.

  vtkSMPropertyHelper(this->ContourFilter,"ContourValues").Set(
    &vectorValues[0], values.size());
  this->ContourFilter->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void ModuleContour::addToPanel(pqProxiesWidget* panel)
{
  Q_ASSERT(this->ContourFilter);
  Q_ASSERT(this->ContourRepresentation);

  QStringList contourProperties;
  contourProperties << "ContourValues";
  panel->addProxy(this->ContourFilter, "Contour", contourProperties, true);

  QStringList contourRepresentationProperties;
  contourRepresentationProperties
    << "Representation"
    << "Opacity"
    << "Specular";
  panel->addProxy(this->ContourRepresentation, "Appearance", contourRepresentationProperties, true);

  this->Superclass::addToPanel(panel);
}

} // end of namespace TEM
