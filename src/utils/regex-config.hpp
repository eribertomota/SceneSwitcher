#pragma once
#include "obs-data.h"

#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRegularExpression>

namespace advss {

class RegexConfigWidget;
class RegexConfigDialog;

class RegexConfig {
public:
	void Save(obs_data_t *obj, const char *name = "regexConfig") const;
	void Load(obs_data_t *obj, const char *name = "regexConfig");

	bool Enabled() const { return _enable; }
	void SetEnabled(bool enable) { _enable = enable; }
	void CreateBackwardsCompatibleRegex(bool, bool = true);
	QRegularExpression::PatternOptions GetPatternOptions() const
	{
		return _options;
	};
	QRegularExpression GetRegularExpression(const QString &) const;
	QRegularExpression GetRegularExpression(const std::string &) const;

	static RegexConfig PartialMatchRegexConfig();

private:
	bool _enable = false;
	bool _partialMatch = false;
	QRegularExpression::PatternOptions _options =
		QRegularExpression::NoPatternOption;
	friend RegexConfigWidget;
	friend RegexConfigDialog;
};

class RegexConfigWidget : public QWidget {
	Q_OBJECT
public:
	RegexConfigWidget(QWidget *parent = nullptr);
	void SetRegexConfig(const RegexConfig &);

public slots:
	void EnableChanged(bool);
	void OpenSettingsClicked();
signals:
	void RegexConfigChanged(RegexConfig);

private:
	void SetVisibility();

	QPushButton *_openSettings;
	QPushButton *_enable;
	RegexConfig _config;
};

class RegexConfigDialog : public QDialog {
	Q_OBJECT

public:
	RegexConfigDialog(QWidget *parent, const RegexConfig &);
	static bool AskForSettings(QWidget *parent, RegexConfig &settings);

private:
	static void SetOption(RegexConfig &, QRegularExpression::PatternOptions,
			      QCheckBox *);

	QCheckBox *_partialMatch;
	QCheckBox *_caseInsensitive;
	QCheckBox *_dotMatch;
	QCheckBox *_multiLine;
	QCheckBox *_extendedPattern;
	QDialogButtonBox *_buttonbox;
};

} // namespace advss
