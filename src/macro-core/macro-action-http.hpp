#pragma once
#include "macro-action-edit.hpp"
#include "resizing-text-edit.hpp"
#include "duration-control.hpp"

#include <QLineEdit>
#include <QComboBox>

class MacroActionHttp : public MacroAction {
public:
	MacroActionHttp(Macro *m) : MacroAction(m) {}
	bool PerformAction();
	void LogAction();
	bool Save(obs_data_t *obj);
	bool Load(obs_data_t *obj);
	std::string GetShortDesc();
	std::string GetId() { return id; };
	static std::shared_ptr<MacroAction> Create(Macro *m)
	{
		return std::make_shared<MacroActionHttp>(m);
	}

	enum class Method {
		GET = 0,
		POST,
	};

	std::string _url = obs_module_text("AdvSceneSwitcher.enterURL");
	std::string _data = obs_module_text("AdvSceneSwitcher.enterText");
	Method _method = Method::GET;
	Duration _timeout;

private:
	void Get();
	void Post();

	static bool _registered;
	static const std::string id;
};

class MacroActionHttpEdit : public QWidget {
	Q_OBJECT

public:
	MacroActionHttpEdit(
		QWidget *parent,
		std::shared_ptr<MacroActionHttp> entryData = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroAction> action)
	{
		return new MacroActionHttpEdit(
			parent,
			std::dynamic_pointer_cast<MacroActionHttp>(action));
	}

private slots:
	void DataChanged();
	void URLChanged();
	void MethodChanged(int);
	void TimeoutChanged(double seconds);
signals:
	void HeaderInfoChanged(const QString &);

protected:
	std::shared_ptr<MacroActionHttp> _entryData;

private:
	void SetWidgetVisibility();

	QLineEdit *_url;
	QComboBox *_methods;
	ResizingPlainTextEdit *_data;
	DurationSelection *_timeout;
	bool _loading = true;
};