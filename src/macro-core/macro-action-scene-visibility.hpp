#pragma once
#include "macro-action-edit.hpp"
#include "scene-selection.hpp"
#include "scene-item-selection.hpp"

#include <QSpinBox>

namespace advss {

enum class SceneVisibilityAction {
	SHOW,
	HIDE,
	TOGGLE,
};

enum class SceneItemSourceType {
	SOURCE,
	SOURCE_GROUP,
};

class MacroActionSceneVisibility : public MacroAction {
public:
	MacroActionSceneVisibility(Macro *m) : MacroAction(m) {}
	bool PerformAction();
	void LogAction() const;
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string GetShortDesc() const;
	std::string GetId() const { return id; };
	static std::shared_ptr<MacroAction> Create(Macro *m)
	{
		return std::make_shared<MacroActionSceneVisibility>(m);
	}

	SceneSelection _scene;
	SceneItemSourceType _sourceType = SceneItemSourceType::SOURCE;
	SceneItemSelection _source;
	std::string _sourceGroup = "";
	SceneVisibilityAction _action = SceneVisibilityAction::SHOW;

private:
	static bool _registered;
	static const std::string id;
};

class MacroActionSceneVisibilityEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionSceneVisibilityEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionSceneVisibility> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionSceneVisibilityEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionSceneVisibility>(
				action));
	}

private slots:
	void SceneChanged(const SceneSelection &);
	void SourceTypeChanged(int value);
	void SourceChanged(const SceneItemSelection &);
	void SourceGroupChanged(const QString &text);
	void ActionChanged(int value);
signals:
	void HeaderInfoChanged(const QString &);

protected:
	SceneSelectionWidget *_scenes;
	QComboBox *_sourceTypes;
	SceneItemSelectionWidget *_sources;
	QComboBox *_sourceGroups;
	QComboBox *_actions;
	std::shared_ptr<MacroActionSceneVisibility> _entryData;

private:
	void SetWidgetVisibility();

	QHBoxLayout *_mainLayout;
	bool _loading = true;
};

} // namespace advss
