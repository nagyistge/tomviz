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
#include "ModulePropertiesPanel.h"
#include "ui_ModulePropertiesPanel.h"

#include "ActiveObjects.h"
#include "Module.h"
#include "ModuleManager.h"
#include "Utilities.h"
#include "pqView.h"
#include "vtkSMViewProxy.h"

namespace {
void deleteLayoutContents(QLayout* layout)
{
  while (layout && layout->count() > 0) {
    QLayoutItem* item = layout->itemAt(0);
    layout->removeItem(item);
    if (item) {
      if (item->widget()) {
          //-----------------------------------------------------------------------------
        delete item->widget();
        delete item;
      } else if (item->layout()) {
        deleteLayoutContents(item->layout());
        delete item->layout();
      }
    }
  }
}
}
namespace tomviz {

class ModulePropertiesPanel::MPPInternals
{
public:
  Ui::ModulePropertiesPanel Ui;
  QPointer<Module> ActiveModule;
};

ModulePropertiesPanel::ModulePropertiesPanel(QWidget* parentObject)
  : Superclass(parentObject),
    Internals(new ModulePropertiesPanel::MPPInternals())
{
  Ui::ModulePropertiesPanel& ui = this->Internals->Ui;
  ui.setupUi(this);

  // Show active module in the "Module Properties" panel.
  this->connect(&ActiveObjects::instance(), SIGNAL(moduleChanged(Module*)),
                SLOT(setModule(Module*)));
  this->connect(&ActiveObjects::instance(),
                SIGNAL(viewChanged(vtkSMViewProxy*)),
                SLOT(setView(vtkSMViewProxy*)));

  /* Disabled the search box for now, uncomment to enable again.
    this->connect(ui.SearchBox, SIGNAL(advancedSearchActivated(bool)),
                  SLOT(updatePanel()));
    this->connect(ui.SearchBox, SIGNAL(textChanged(const QString&)),
                  SLOT(updatePanel()));
  */

  this->connect(ui.DetachColorMap, SIGNAL(clicked(bool)),
                SLOT(detachColorMap(bool)));
}

ModulePropertiesPanel::~ModulePropertiesPanel()
{
}

void ModulePropertiesPanel::setModule(Module* module)
{
  if (module != this->Internals->ActiveModule) {
    if (this->Internals->ActiveModule) {
      DataSource* dataSource = this->Internals->ActiveModule->dataSource();
      QObject::disconnect(dataSource, SIGNAL(dataChanged()), this,
                          SLOT(updatePanel()));
      QObject::disconnect(this->Internals->ActiveModule, SIGNAL(renderNeeded()),
                          this, SLOT(render()));
    }

    if (module) {
      DataSource* dataSource = module->dataSource();
      QObject::connect(dataSource, SIGNAL(dataChanged()), this,
                       SLOT(updatePanel()));
      QObject::connect(module, SIGNAL(renderNeeded()), this, SLOT(render()));
    }
  }

  this->Internals->ActiveModule = module;
  Ui::ModulePropertiesPanel& ui = this->Internals->Ui;

  deleteLayoutContents(ui.PropertiesWidget->layout());

  ui.DetachColorMap->setVisible(false);
  if (module) {
    module->addToPanel(ui.PropertiesWidget);
    if (module->isColorMapNeeded()) {
      ui.DetachColorMap->setVisible(true);
      ui.DetachColorMap->setChecked(module->useDetachedColorMap());
    }
  }
  ui.PropertiesWidget->layout()->update();
  this->updatePanel();
}

void ModulePropertiesPanel::setView(vtkSMViewProxy* vtkNotUsed(view))
{
}

void ModulePropertiesPanel::updatePanel()
{
}

void ModulePropertiesPanel::render()
{
  pqView* view =
    tomviz::convert<pqView*>(ActiveObjects::instance().activeView());
  if (view) {
    view->render();
  }
}

void ModulePropertiesPanel::detachColorMap(bool val)
{
  Module* module = this->Internals->ActiveModule;
  if (module) {
    module->setUseDetachedColorMap(val);
    this->setModule(module); // refreshes the module.
    this->render();
  }
}

}
