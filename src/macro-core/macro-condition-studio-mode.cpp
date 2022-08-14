#include "macro-condition-edit.hpp"
#include "macro-condition-studio-mode.hpp"
#include "utility.hpp"
#include "advanced-scene-switcher.hpp"

const std::string MacroConditionStudioMode::id = "studio_mode";

bool MacroConditionStudioMode::_registered = MacroConditionFactory::Register(
	MacroConditionStudioMode::id,
	{MacroConditionStudioMode::Create, MacroConditionStudioModeEdit::Create,
	 "AdvSceneSwitcher.condition.studioMode"});

static std::map<StudioModeCondition, std::string> studioModeConditionTypes = {
	{StudioModeCondition::STUDIO_MODE_ACTIVE,
	 "AdvSceneSwitcher.condition.studioMode.state.active"},
	{StudioModeCondition::STUDIO_MODE_NOT_ACTIVE,
	 "AdvSceneSwitcher.condition.studioMode.state.notActive"},
	{StudioModeCondition::PREVIEW_SCENE,
	 "AdvSceneSwitcher.condition.studioMode.state.previewScene"},
};

bool MacroConditionStudioMode::CheckCondition()
{
	bool ret = false;
	switch (_condition) {
	case StudioModeCondition::STUDIO_MODE_ACTIVE:
		ret = obs_frontend_preview_program_mode_active();
		break;
	case StudioModeCondition::STUDIO_MODE_NOT_ACTIVE:
		ret = !obs_frontend_preview_program_mode_active();
		break;
	case StudioModeCondition::PREVIEW_SCENE: {
		auto s = obs_frontend_get_current_preview_scene();
		auto scene = obs_source_get_weak_source(s);
		ret = _scene.GetScene() == scene;
		obs_weak_source_release(scene);
		obs_source_release(s);
		break;
	}
	default:
		break;
	}
	return ret;
}

bool MacroConditionStudioMode::Save(obs_data_t *obj)
{
	MacroCondition::Save(obj);
	obs_data_set_int(obj, "condition", static_cast<int>(_condition));
	_scene.Save(obj);
	return true;
}

bool MacroConditionStudioMode::Load(obs_data_t *obj)
{
	MacroCondition::Load(obj);
	_condition = static_cast<StudioModeCondition>(
		obs_data_get_int(obj, "condition"));
	_scene.Load(obj);
	return true;
}

std::string MacroConditionStudioMode::GetShortDesc()
{
	if (_condition == StudioModeCondition::PREVIEW_SCENE) {
		return _scene.ToString();
	}
	return "";
}

static inline void populateConditionSelection(QComboBox *list)
{
	for (auto entry : studioModeConditionTypes) {
		list->addItem(obs_module_text(entry.second.c_str()));
	}
}

MacroConditionStudioModeEdit::MacroConditionStudioModeEdit(
	QWidget *parent, std::shared_ptr<MacroConditionStudioMode> entryData)
	: QWidget(parent)
{
	_condition = new QComboBox();
	_scenes = new SceneSelectionWidget(window());

	populateConditionSelection(_condition);

	QWidget::connect(_condition, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(ConditionChanged(int)));
	QWidget::connect(_scenes, SIGNAL(SceneChanged(const SceneSelection &)),
			 this, SLOT(SceneChanged(const SceneSelection &)));

	QHBoxLayout *layout = new QHBoxLayout;
	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{conditions}}", _condition},
		{"{{scenes}}", _scenes},
	};
	placeWidgets(
		obs_module_text("AdvSceneSwitcher.condition.studioMode.entry"),
		layout, widgetPlaceholders);
	setLayout(layout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;
}

void MacroConditionStudioModeEdit::ConditionChanged(int cond)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_condition = static_cast<StudioModeCondition>(cond);
	SetWidgetVisibility();
	emit HeaderInfoChanged(
		QString::fromStdString(_entryData->GetShortDesc()));
}

void MacroConditionStudioModeEdit::SceneChanged(const SceneSelection &s)
{
	if (_loading || !_entryData) {
		return;
	}

	std::lock_guard<std::mutex> lock(switcher->m);
	_entryData->_scene = s;
	emit HeaderInfoChanged(
		QString::fromStdString(_entryData->GetShortDesc()));
}

void MacroConditionStudioModeEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}

	_condition->setCurrentIndex(static_cast<int>(_entryData->_condition));
	_scenes->SetScene(_entryData->_scene);
	SetWidgetVisibility();
}

void MacroConditionStudioModeEdit::SetWidgetVisibility()
{
	if (!_entryData) {
		return;
	}

	_scenes->setVisible(_entryData->_condition ==
			    StudioModeCondition::PREVIEW_SCENE);
}