#pragma once
#include "macro.hpp"
#include "variable-text-edit.hpp"
#include "regex-config.hpp"

#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>

enum class FilterCondition {
	ENABLED,
	DISABLED,
	SETTINGS,
};

class MacroConditionFilter : public MacroCondition {
public:
	MacroConditionFilter(Macro *m) : MacroCondition(m) {}
	bool CheckCondition();
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string GetShortDesc() const;
	std::string GetId() const { return id; };
	static std::shared_ptr<MacroCondition> Create(Macro *m)
	{
		return std::make_shared<MacroConditionFilter>(m);
	}

	OBSWeakSource _source;
	OBSWeakSource _filter;
	FilterCondition _condition = FilterCondition::ENABLED;
	VariableResolvingString _settings = "";
	RegexConfig _regex;

private:
	static bool _registered;
	static const std::string id;
};

class MacroConditionFilterEdit : public QWidget {
	Q_OBJECT

public:
	MacroConditionFilterEdit(
		QWidget *parent,
		std::shared_ptr<MacroConditionFilter> cond = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroCondition> cond)
	{
		return new MacroConditionFilterEdit(
			parent,
			std::dynamic_pointer_cast<MacroConditionFilter>(cond));
	}

private slots:
	void SourceChanged(const QString &text);
	void FilterChanged(const QString &text);
	void ConditionChanged(int cond);
	void GetSettingsClicked();
	void SettingsChanged();
	void RegexChanged(RegexConfig);
signals:
	void HeaderInfoChanged(const QString &);

protected:
	QComboBox *_sources;
	QComboBox *_filters;
	QComboBox *_conditions;
	QPushButton *_getSettings;
	VariableTextEdit *_settings;
	RegexConfigWidget *_regex;

	std::shared_ptr<MacroConditionFilter> _entryData;

private:
	void SetSettingsSelectionVisible(bool visible);
	bool _loading = true;
};
