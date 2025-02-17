#pragma once
#include "scene-selection.hpp"
#include "variable.hpp"
#include "utility.hpp"

#include <QComboBox>
#include <obs-data.h>

namespace advss {

class SceneItemSelection {
public:
	void Save(obs_data_t *obj,
		  const char *name = "sceneItemSelection") const;
	void Load(obs_data_t *obj, const char *name = "sceneItemSelection");
	// TODO: Remove in future version
	void Load(obs_data_t *obj, const char *name, const char *targetName,
		  const char *idxName);

	enum class Type {
		SOURCE,
		VARIABLE,
	};
	enum class IdxType {
		ALL,
		ANY,
		INDIVIDUAL,
	};

	Type GetType() const { return _type; }
	IdxType GetIndexType() const { return _idxType; }
	std::vector<obs_scene_item *> GetSceneItems(SceneSelection &s) const;
	std::string ToString(bool resolve = false) const;

private:
	OBSWeakSource _sceneItem;
	std::weak_ptr<Variable> _variable;
	Type _type = Type::SOURCE;
	IdxType _idxType = IdxType::ALL;
	int _idx = 0; // Multiple items with the same name can exist
	friend class SceneItemSelectionWidget;
};

class SceneItemSelectionWidget : public QWidget {
	Q_OBJECT

public:
	enum class Placeholder { ALL, ANY };
	SceneItemSelectionWidget(QWidget *parent, bool addPlaceholder = true,
				 Placeholder placeholderType = Placeholder::ALL);
	void SetSceneItem(const SceneItemSelection &);
	void SetScene(const SceneSelection &);
	void ShowPlaceholder(bool);
	void SetPlaceholderType(Placeholder t, bool resetSelection = true);
signals:
	void SceneItemChanged(const SceneItemSelection &);

private slots:
	void SceneChanged(const SceneSelection &);
	void SelectionChanged(const QString &name);
	void IdxChanged(int);
	void ItemAdd(const QString &name);
	void ItemRemove(const QString &name);
	void ItemRename(const QString &oldName, const QString &newName);

private:
	void Reset();
	void PopulateItemSelection();
	void SetupIdxSelection(int);

	QComboBox *_sceneItems;
	QComboBox *_idx;

	SceneSelection _scene;
	SceneItemSelection _currentSelection;
	bool _hasPlaceholderEntry = false;
	Placeholder _placeholder = Placeholder::ALL;

	// Order of entries
	// 1. "select entry" entry
	// 2. Variables
	// 3. Scene items
	const int _selectIdx = 0;
	int _variablesEndIdx = -1;
	int _itemsEndIdx = -1;
};

} // namespace advss
