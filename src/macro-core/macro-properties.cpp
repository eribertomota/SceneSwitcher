#include "macro-properties.hpp"
#include "obs-module-helper.hpp"
#include "utility.hpp"

#include <QVBoxLayout>
#include <QDialogButtonBox>

namespace advss {

void MacroProperties::Save(obs_data_t *obj) const
{
	auto data = obs_data_create();
	obs_data_set_bool(data, "highlightExecuted", _highlightExecuted);
	obs_data_set_bool(data, "highlightConditions", _highlightConditions);
	obs_data_set_bool(data, "highlightActions", _highlightActions);
	obs_data_set_bool(data, "newMacroRegisterHotkey",
			  _newMacroRegisterHotkeys);
	obs_data_set_obj(obj, "macroProperties", data);
	obs_data_release(data);
}

void MacroProperties::Load(obs_data_t *obj)
{
	auto data = obs_data_get_obj(obj, "macroProperties");
	_highlightExecuted = obs_data_get_bool(data, "highlightExecuted");
	_highlightConditions = obs_data_get_bool(data, "highlightConditions");
	_highlightActions = obs_data_get_bool(data, "highlightActions");
	_newMacroRegisterHotkeys =
		obs_data_get_bool(data, "newMacroRegisterHotkey");
	obs_data_release(data);
}

static void setGridLayoutRowVisible(QGridLayout *layout, int row, bool visible)
{
	for (int col = 0; col < layout->columnCount(); col++) {
		auto item = layout->itemAtPosition(row, col);
		if (!item) {
			continue;
		}

		auto rowLayout = item->layout();
		if (rowLayout) {
			SetLayoutVisible(rowLayout, visible);
		}

		auto widget = item->widget();
		if (widget) {
			widget->setVisible(visible);
		}
	}

	if (!visible) {
		layout->setRowMinimumHeight(row, 0);
	}
}

MacroPropertiesDialog::MacroPropertiesDialog(QWidget *parent,
					     const MacroProperties &prop,
					     Macro *macro)
	: QDialog(parent),
	  _executed(new QCheckBox(obs_module_text(
		  "AdvSceneSwitcher.macroTab.highlightExecutedMacros"))),
	  _conditions(new QCheckBox(obs_module_text(
		  "AdvSceneSwitcher.macroTab.highlightTrueConditions"))),
	  _actions(new QCheckBox(obs_module_text(
		  "AdvSceneSwitcher.macroTab.highlightPerformedActions"))),
	  _newMacroRegisterHotkeys(new QCheckBox(obs_module_text(
		  "AdvSceneSwitcher.macroTab.newMacroRegisterHotkey"))),
	  _currentMacroRegisterHotkeys(new QCheckBox(obs_module_text(
		  "AdvSceneSwitcher.macroTab.currentDisableHotkeys"))),
	  _currentMacroRegisterDock(new QCheckBox(obs_module_text(
		  "AdvSceneSwitcher.macroTab.currentRegisterDock"))),
	  _currentMacroDockAddRunButton(new QCheckBox(obs_module_text(
		  "AdvSceneSwitcher.macroTab.currentDockAddRunButton"))),
	  _currentMacroDockAddPauseButton(new QCheckBox(obs_module_text(
		  "AdvSceneSwitcher.macroTab.currentDockAddPauseButton"))),
	  _runButtonText(new QLineEdit()),
	  _pauseButtonText(new QLineEdit()),
	  _unpauseButtonText(new QLineEdit()),
	  _dockOptions(new QGroupBox(
		  obs_module_text("AdvSceneSwitcher.macroTab.dockSettings"))),
	  _dockLayout(new QGridLayout())
{
	setModal(true);
	setWindowModality(Qt::WindowModality::WindowModal);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	auto highlightOptions = new QGroupBox(
		obs_module_text("AdvSceneSwitcher.macroTab.highlightSettings"));
	auto highlightLayout = new QVBoxLayout;
	highlightLayout->addWidget(_executed);
	highlightLayout->addWidget(_conditions);
	highlightLayout->addWidget(_actions);
	highlightOptions->setLayout(highlightLayout);

	auto hotkeyOptions = new QGroupBox(
		obs_module_text("AdvSceneSwitcher.macroTab.hotkeySettings"));
	auto hotkeyLayout = new QVBoxLayout;
	hotkeyLayout->addWidget(_newMacroRegisterHotkeys);
	hotkeyLayout->addWidget(_currentMacroRegisterHotkeys);
	hotkeyOptions->setLayout(hotkeyLayout);

	int row = 0;
	_dockLayout->addWidget(_currentMacroRegisterDock, row, 1, 1, 2);
	row++;
	_dockLayout->addWidget(_currentMacroDockAddRunButton, row, 1, 1, 2);
	row++;
	_dockLayout->addWidget(
		new QLabel(obs_module_text(
			"AdvSceneSwitcher.macroTab.currentDockButtonText.run")),
		row, 1);
	_dockLayout->addWidget(_runButtonText, row, 2);
	_runButtonTextRow = row;
	row++;
	_dockLayout->addWidget(_currentMacroDockAddPauseButton, row, 1, 1, 2);
	row++;
	_dockLayout->addWidget(
		new QLabel(obs_module_text(
			"AdvSceneSwitcher.macroTab.currentDockButtonText.pause")),
		row, 1);
	_dockLayout->addWidget(_pauseButtonText, row, 2);
	_pauseButtonTextRow = row;
	row++;
	_dockLayout->addWidget(
		new QLabel(obs_module_text(
			"AdvSceneSwitcher.macroTab.currentDockButtonText.unpause")),
		row, 1);
	_dockLayout->addWidget(_unpauseButtonText, row, 2);
	_unpauseButtonTextRow = row;
	_dockOptions->setLayout(_dockLayout);

	QDialogButtonBox *buttonbox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonbox->setCenterButtons(true);
	connect(buttonbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(_currentMacroRegisterDock, &QCheckBox::stateChanged, this,
		&MacroPropertiesDialog::DockEnableChanged);
	connect(_currentMacroDockAddRunButton, &QCheckBox::stateChanged, this,
		&MacroPropertiesDialog::RunButtonEnableChanged);
	connect(_currentMacroDockAddPauseButton, &QCheckBox::stateChanged, this,
		&MacroPropertiesDialog::PauseButtonEnableChanged);

	auto layout = new QVBoxLayout;
	layout->addWidget(highlightOptions);
	layout->addWidget(hotkeyOptions);
	layout->addWidget(_dockOptions);
	layout->addWidget(buttonbox);
	setLayout(layout);

	_executed->setChecked(prop._highlightExecuted);
	_conditions->setChecked(prop._highlightConditions);
	_actions->setChecked(prop._highlightActions);
	_newMacroRegisterHotkeys->setChecked(prop._newMacroRegisterHotkeys);
	if (!macro || macro->IsGroup()) {
		hotkeyOptions->hide();
		_dockOptions->hide();
		return;
	}
	_currentMacroRegisterHotkeys->setChecked(macro->PauseHotkeysEnabled());
	const bool dockEnabled = macro->DockEnabled();
	_currentMacroRegisterDock->setChecked(dockEnabled);
	_currentMacroDockAddRunButton->setChecked(macro->DockHasRunButton());
	_currentMacroDockAddPauseButton->setChecked(
		macro->DockHasPauseButton());
	_runButtonText->setText(QString::fromStdString(macro->RunButtonText()));
	_pauseButtonText->setText(
		QString::fromStdString(macro->PauseButtonText()));
	_unpauseButtonText->setText(
		QString::fromStdString(macro->UnpauseButtonText()));

	_currentMacroDockAddRunButton->setVisible(dockEnabled);
	_currentMacroDockAddPauseButton->setVisible(dockEnabled);
	setGridLayoutRowVisible(_dockLayout, _runButtonTextRow,
				dockEnabled && macro->DockHasRunButton());
	setGridLayoutRowVisible(_dockLayout, _pauseButtonTextRow,
				dockEnabled && macro->DockHasPauseButton());
	setGridLayoutRowVisible(_dockLayout, _unpauseButtonTextRow,
				dockEnabled && macro->DockHasPauseButton());
	MinimizeSizeOfColumn(_dockLayout, 0);
	Resize();
}

void MacroPropertiesDialog::DockEnableChanged(int enabled)
{
	_currentMacroDockAddRunButton->setVisible(enabled);
	_currentMacroDockAddPauseButton->setVisible(enabled);
	setGridLayoutRowVisible(_dockLayout, _runButtonTextRow, enabled);
	setGridLayoutRowVisible(_dockLayout, _pauseButtonTextRow, enabled);
	setGridLayoutRowVisible(_dockLayout, _unpauseButtonTextRow, enabled);
	Resize();
}

void MacroPropertiesDialog::RunButtonEnableChanged(int enabled)
{
	setGridLayoutRowVisible(_dockLayout, _runButtonTextRow, enabled);
	Resize();
}

void MacroPropertiesDialog::PauseButtonEnableChanged(int enabled)
{
	setGridLayoutRowVisible(_dockLayout, _pauseButtonTextRow, enabled);
	setGridLayoutRowVisible(_dockLayout, _unpauseButtonTextRow, enabled);
	Resize();
}

void MacroPropertiesDialog::Resize()
{
	_dockOptions->adjustSize();
	_dockOptions->updateGeometry();
	adjustSize();
	updateGeometry();
}

bool MacroPropertiesDialog::AskForSettings(QWidget *parent,
					   MacroProperties &userInput,
					   Macro *macro)
{
	MacroPropertiesDialog dialog(parent, userInput, macro);
	dialog.setWindowTitle(obs_module_text("AdvSceneSwitcher.windowTitle"));
	if (dialog.exec() != DialogCode::Accepted) {
		return false;
	}
	userInput._highlightExecuted = dialog._executed->isChecked();
	userInput._highlightConditions = dialog._conditions->isChecked();
	userInput._highlightActions = dialog._actions->isChecked();
	userInput._newMacroRegisterHotkeys =
		dialog._newMacroRegisterHotkeys->isChecked();
	if (!macro) {
		return true;
	}

	macro->EnablePauseHotkeys(
		dialog._currentMacroRegisterHotkeys->isChecked());
	macro->EnableDock(dialog._currentMacroRegisterDock->isChecked());
	macro->SetDockHasRunButton(
		dialog._currentMacroDockAddRunButton->isChecked());
	macro->SetDockHasPauseButton(
		dialog._currentMacroDockAddPauseButton->isChecked());
	macro->SetRunButtonText(dialog._runButtonText->text().toStdString());
	macro->SetPauseButtonText(
		dialog._pauseButtonText->text().toStdString());
	macro->SetUnpauseButtonText(
		dialog._unpauseButtonText->text().toStdString());
	return true;
}

} // namespace advss
