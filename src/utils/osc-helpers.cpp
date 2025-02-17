#include "osc-helpers.hpp"
#include "obs-module-helper.hpp"
#include "utility.hpp"

#include <string.h>
#include <QGroupBox>

namespace advss {

std::unordered_map<size_t, OSCMessageElement::TypeInfo>
	OSCMessageElement::_typeNames = {
		{std::variant_npos,
		 {"AdvSceneSwitcher.osc.message.type.none", "-"}},
		{0, {"AdvSceneSwitcher.osc.message.type.int", "i"}},
		{1, {"AdvSceneSwitcher.osc.message.type.float", "f"}},
		{2, {"AdvSceneSwitcher.osc.message.type.string", "s"}},
		{3, {"AdvSceneSwitcher.osc.message.type.binaryBlob", "b"}},
		{4, {"AdvSceneSwitcher.osc.message.type.true", "T"}},
		{5, {"AdvSceneSwitcher.osc.message.type.false", "F"}},
		{6, {"AdvSceneSwitcher.osc.message.type.infinity", "I"}},
		{7, {"AdvSceneSwitcher.osc.message.type.null", "N"}},
};

// Based on https://github.com/mhroth/tinyosc
struct FillMessageElementBufferVisitor {
	std::vector<char> &buffer;
	uint32_t &curOffset;

	bool success = false;

	void operator()(const StringVariable &value)
	{
		std::string string = value;
		int length = (int)strlen(string.c_str());
		if (curOffset + length >= buffer.size()) {
			buffer.resize(curOffset + length + 1);
		}
		strncpy(buffer.data() + curOffset, string.c_str(),
			buffer.size() - curOffset - length);
		curOffset = (curOffset + 4 + length) & ~0x3;
		success = true;
	}
	void operator()(const IntVariable &value)
	{
		if (curOffset + 4 > buffer.size()) {
			buffer.resize(curOffset + 4);
		}
		int32_t k = value;
		*((uint32_t *)(buffer.data() + curOffset)) = htonl(k);
		curOffset += 4;
		success = true;
	}
	void operator()(const DoubleVariable &value)
	{
		if (curOffset + 4 > buffer.size()) {
			buffer.resize(curOffset + 4);
		}
		const float f = value;
		*((uint32_t *)(buffer.data() + curOffset)) =
			htonl(*((uint32_t *)&f));
		curOffset += 4;
		success = true;
	}
	void operator()(const OSCBlob &value)
	{
		if (curOffset + 4 > buffer.size()) {
			buffer.resize(curOffset + 4);
		}
		auto blob = value.GetBinary();
		if (!blob.has_value()) {
			return;
		}
		if (curOffset + 4 + blob->size() > buffer.size()) {
			buffer.resize(curOffset + 4 + blob->size());
		}
		*((uint32_t *)(buffer.data() + curOffset)) =
			htonl(blob->size());
		curOffset += 4;
		memcpy(buffer.data() + curOffset, blob->data(), blob->size());
		curOffset = (curOffset + 3 + blob->size()) & ~0x3;
		success = true;
	}
	void operator()(const OSCTrue &) { success = true; }
	void operator()(const OSCFalse &) { success = true; }
	void operator()(const OSCInfinity &) { success = true; }
	void operator()(const OSCNull &) { success = true; }
};

OSCBlob::OSCBlob(const std::string &stringRepresentation)
	: _stringRep(stringRepresentation)
{
}

void OSCBlob::SetStringRepresentation(const StringVariable &s)
{
	_stringRep = s;
}

std::string OSCBlob::GetStringRepresentation() const
{
	return _stringRep;
}

std::optional<std::vector<char>> OSCBlob::GetBinary() const
{
	std::vector<char> bytes;
	std::string hexString = _stringRep;

	for (std::size_t i = 2; i < hexString.size(); i += 4) {
		try {
			auto byteString = hexString.substr(i, 2);
			int value = std::stoi(byteString, nullptr, 16);
			bytes.push_back(static_cast<char>(value));
		} catch (const std::exception &e) {
			blog(LOG_WARNING,
			     "failed to convert hex \"%s\" to binary: %s",
			     hexString.c_str(), e.what());
			return {};
		}
	}

	return bytes;
}

void OSCBlob::Save(obs_data_t *obj, const char *name) const
{
	_stringRep.Save(obj, name);
}

void OSCBlob::Load(obs_data_t *obj, const char *name)
{
	_stringRep.Load(obj, name);
}

void OSCTrue::Save(obs_data_t *obj, const char *name) const
{
	obs_data_set_bool(obj, name, true);
}

void OSCFalse::Save(obs_data_t *obj, const char *name) const
{
	obs_data_set_bool(obj, name, true);
}

void OSCInfinity::Save(obs_data_t *obj, const char *name) const
{
	obs_data_set_bool(obj, name, true);
}

void OSCNull::Save(obs_data_t *obj, const char *name) const
{
	obs_data_set_bool(obj, name, true);
}

std::optional<std::vector<char>> OSCMessage::GetBuffer() const
{
	if (std::string(_address).empty()) {
		return {};
	}

	std::vector<char> buffer(128, 0);
	uint32_t currentOffset = (uint32_t)strlen(_address.c_str());
	if (currentOffset > buffer.size()) {
		buffer.resize(buffer.size() * 2);
	}
	strncpy(buffer.data(), _address.c_str(), buffer.size());
	currentOffset = (currentOffset + 4) & ~0x3;

	std::string typeTags;
	for (const auto &e : _elements) {
		typeTags += e.GetTypeTag();
	}

	buffer.at(currentOffset++) = ',';
	int length = (int)strlen(typeTags.c_str());
	if (currentOffset + length >= buffer.size()) {
		buffer.resize(buffer.size() * 2);
	}
	strncpy(buffer.data() + currentOffset, typeTags.c_str(),
		buffer.size() - currentOffset - length);
	currentOffset = (currentOffset + 4 + length) & ~0x3;

	for (const auto &e : _elements) {
		const auto &value = e._value;
		FillMessageElementBufferVisitor visitor{buffer, currentOffset};
		std::visit(visitor, value);
		if (!visitor.success) {
			return {};
		}
	}

	buffer.resize(currentOffset);
	return buffer;
}

const char *OSCMessageElement::GetTypeTag() const
{
	return GetTypeTag(*this);
}

const char *OSCMessageElement::GetTypeName() const
{
	return GetTypeName(*this);
}

const char *OSCMessageElement::GetTypeTag(const OSCMessageElement &element)
{
	return _typeNames.at(element._value.index()).tag;
}

const char *OSCMessageElement::GetTypeName(const OSCMessageElement &element)
{
	return obs_module_text(
		_typeNames.at(element._value.index()).localizedName);
}

void OSCMessageElement::Save(obs_data_t *obj) const
{
	std::visit(
		[obj](auto &&arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, StringVariable>) {
				arg.Save(obj, "strValue");
			} else if constexpr (std::is_same_v<T, IntVariable>) {
				arg.Save(obj, "intValue");
			} else if constexpr (std::is_same_v<T, DoubleVariable>) {
				arg.Save(obj, "floatValue");
			} else if constexpr (std::is_same_v<T, OSCBlob>) {
				arg.Save(obj, "binaryValue");
			} else if constexpr (std::is_same_v<T, OSCTrue>) {
				arg.Save(obj, "trueValue");
			} else if constexpr (std::is_same_v<T, OSCFalse>) {
				arg.Save(obj, "falseValue");
			} else if constexpr (std::is_same_v<T, OSCInfinity>) {
				arg.Save(obj, "infiniteValue");
			} else if constexpr (std::is_same_v<T, OSCNull>) {
				arg.Save(obj, "nullValue");
			} else {
				blog(LOG_WARNING,
				     "cannot save unkown OSCMessageElement");
			}
		},
		_value);
}

void OSCMessageElement::Load(obs_data_t *obj)
{
	if (obs_data_has_user_value(obj, "strValue")) {
		StringVariable string;
		string.Load(obj, "strValue");
		_value = string;
	} else if (obs_data_has_user_value(obj, "intValue")) {
		NumberVariable<int> intValue;
		intValue.Load(obj, "intValue");
		_value = intValue;
	} else if (obs_data_has_user_value(obj, "floatValue")) {
		NumberVariable<double> floatValue;
		floatValue.Load(obj, "floatValue");
		_value = floatValue;
	} else if (obs_data_has_user_value(obj, "binaryValue")) {
		OSCBlob binValue;
		binValue.Load(obj, "binaryValue");
		_value = binValue;
	} else if (obs_data_has_user_value(obj, "trueValue")) {
		_value = OSCTrue();
	} else if (obs_data_has_user_value(obj, "falseValue")) {
		_value = OSCFalse();
	} else if (obs_data_has_user_value(obj, "OSCInfinite")) {
		_value = OSCInfinity();
	} else if (obs_data_has_user_value(obj, "nullValue")) {
		_value = OSCNull();
	} else {
		blog(LOG_WARNING, "cannot load unkown OSCMessageElement");
	}
}

std::string OSCMessageElement::ToString() const
{
	return std::visit(
		[](auto &&arg) -> std::string {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, StringVariable>) {
				return arg;
			} else if constexpr (std::is_same_v<T, OSCBlob> ||
					     std::is_same_v<T, OSCTrue> ||
					     std::is_same_v<T, OSCFalse> ||
					     std::is_same_v<T, OSCInfinity> ||
					     std::is_same_v<T, OSCNull>) {
				return arg.GetStringRepresentation();
			} else {
				return std::to_string(arg.GetValue());
			}
		},
		_value);
}

void OSCMessage::Save(obs_data_t *obj) const
{
	auto data = obs_data_create();
	_address.Save(data, "address");
	auto elements = obs_data_array_create();
	for (const auto &e : _elements) {
		auto array_obj = obs_data_create();
		e.Save(array_obj);
		obs_data_array_push_back(elements, array_obj);
		obs_data_release(array_obj);
	}
	obs_data_set_array(data, "elements", elements);
	obs_data_set_obj(obj, "oscMessage", data);
	obs_data_array_release(elements);
	obs_data_release(data);
}

void OSCMessage::Load(obs_data_t *obj)
{

	auto data = obs_data_get_obj(obj, "oscMessage");
	_address.Load(data, "address");
	_elements.clear();
	auto elements = obs_data_get_array(data, "elements");
	size_t count = obs_data_array_count(elements);
	for (size_t i = 0; i < count; i++) {
		auto array_obj = obs_data_array_item(elements, i);
		OSCMessageElement e;
		e.Load(array_obj);
		_elements.push_back(e);
		obs_data_release(array_obj);
	}
	obs_data_array_release(elements);
	obs_data_release(data);
}

std::string OSCMessage::ToString() const
{
	std::string res = "address: " + std::string(_address) + " message: ";
	for (const auto &e : _elements) {
		res += "[" + e.ToString() + "]";
	}
	return res;
}

OSCMessageElementEdit::OSCMessageElementEdit(QWidget *parent)
	: QWidget(parent),
	  _type(new QComboBox(this)),
	  _intValue(new VariableSpinBox(this)),
	  _doubleValue(new VariableDoubleSpinBox(this)),
	  _text(new VariableLineEdit(this)),
	  _binaryText(new VariableLineEdit(this))
{
	installEventFilter(this);

	_intValue->setMinimum(INT_MIN);
	_intValue->setMaximum(INT_MAX);
	_doubleValue->setMinimum(-9999999999);
	_doubleValue->setMaximum(9999999999);
	_doubleValue->setDecimals(10);

	_intValue->hide();
	_doubleValue->hide();
	_text->hide();
	_binaryText->hide();

	for (int i = 0; i < OSCMessageElement::_typeNames.size() - 1; i++) {
		_type->addItem(obs_module_text(
			OSCMessageElement::_typeNames.at(i).localizedName));
	}
	_type->setCurrentIndex(0);

	QWidget::connect(_type, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(TypeChanged(int)));
	QWidget::connect(
		_doubleValue,
		SIGNAL(NumberVariableChanged(const NumberVariable<double> &)),
		this, SLOT(DoubleChanged(const NumberVariable<double> &)));
	QWidget::connect(
		_intValue,
		SIGNAL(NumberVariableChanged(const NumberVariable<int> &)),
		this, SLOT(IntChanged(const NumberVariable<int> &)));
	QWidget::connect(_text, SIGNAL(editingFinished()), this,
			 SLOT(TextChanged()));
	QWidget::connect(_binaryText, SIGNAL(editingFinished()), this,
			 SLOT(BinaryTextChanged()));

	auto layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(_type, 1);
	layout->addWidget(_intValue, 4);
	layout->addWidget(_doubleValue, 4);
	layout->addWidget(_text, 4);
	layout->addWidget(_binaryText, 4);
	setLayout(layout);
}

void OSCMessageElementEdit::SetMessageElement(const OSCMessageElement &element)
{
	const QSignalBlocker b(this);
	_type->setCurrentText(element.GetTypeName());
	SetVisibility(element);

	if (std::holds_alternative<StringVariable>(element._value)) {
		_text->setText(std::get<StringVariable>(element._value));
	} else if (std::holds_alternative<IntVariable>(element._value)) {
		_intValue->SetValue(std::get<IntVariable>(element._value));
	} else if (std::holds_alternative<DoubleVariable>(element._value)) {
		_doubleValue->SetValue(
			std::get<DoubleVariable>(element._value));
	} else if (std::holds_alternative<OSCBlob>(element._value)) {
		_binaryText->setText(std::get<OSCBlob>(element._value)
					     .GetStringRepresentation());
	}
}

bool OSCMessageElementEdit::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress ||
	    event->type() == QEvent::MouseButtonDblClick) {
		emit Focussed();
	}

	return QWidget::eventFilter(obj, event);
}
void OSCMessageElementEdit::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	QWidgetList childWidgets = findChildren<QWidget *>();
	for (QWidget *childWidget : childWidgets) {
		childWidget->installEventFilter(this);
	}
}

void OSCMessageElementEdit::DoubleChanged(const DoubleVariable &value)
{
	emit ElementValueChanged(OSCMessageElement(value));
}

void OSCMessageElementEdit::IntChanged(const IntVariable &value)
{
	emit ElementValueChanged(OSCMessageElement(value));
}

void OSCMessageElementEdit::TextChanged()
{
	emit ElementValueChanged(StringVariable(_text->text().toStdString()));
}

void OSCMessageElementEdit::BinaryTextChanged()
{
	emit ElementValueChanged(OSCBlob(_binaryText->text().toStdString()));
}

void OSCMessageElementEdit::SetVisibility(const OSCMessageElement &element)
{
	_intValue->hide();
	_doubleValue->hide();
	_text->hide();
	_binaryText->hide();

	if (std::holds_alternative<StringVariable>(element._value)) {
		_text->show();
	} else if (std::holds_alternative<IntVariable>(element._value)) {
		_intValue->show();
	} else if (std::holds_alternative<DoubleVariable>(element._value)) {
		_doubleValue->show();
	} else if (std::holds_alternative<OSCBlob>(element._value)) {
		_binaryText->show();
	}
}

void OSCMessageElementEdit::TypeChanged(int idx)
{
	OSCMessageElement element;
	if (idx == 0) {
		element = OSCMessageElement(IntVariable(0));
	} else if (idx == 1) {
		element = OSCMessageElement(DoubleVariable(0.0));
	} else if (idx == 2) {
		element = OSCMessageElement("value");
	} else if (idx == 3) {
		element = OSCMessageElement(OSCBlob("\\x00\\x01\\x02\\x03"));
	} else if (idx == 4) {
		element = OSCMessageElement(OSCTrue());
	} else if (idx == 5) {
		element = OSCMessageElement(OSCFalse());
	} else if (idx == 6) {
		element = OSCMessageElement(OSCInfinity());
	} else if (idx == 7) {
		element = OSCMessageElement(OSCNull());
	}
	SetVisibility(element);
	SetMessageElement(element);
	emit ElementValueChanged(element);
}

OSCMessageEdit::OSCMessageEdit(QWidget *parent)
	: QWidget(parent),
	  _address(new VariableLineEdit(this)),
	  _elements(new QListWidget()),
	  _add(new QPushButton()),
	  _remove(new QPushButton()),
	  _up(new QPushButton()),
	  _down(new QPushButton())
{
	_elements->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_elements->setAutoScroll(false);

	_add->setMaximumWidth(22);
	_add->setProperty("themeID",
			  QVariant(QString::fromUtf8("addIconSmall")));
	_add->setFlat(true);
	_remove->setMaximumWidth(22);
	_remove->setProperty("themeID",
			     QVariant(QString::fromUtf8("removeIconSmall")));
	_remove->setFlat(true);
	_up->setMaximumWidth(22);
	_up->setProperty("themeID",
			 QVariant(QString::fromUtf8("upArrowIconSmall")));
	_up->setFlat(true);
	_down->setMaximumWidth(22);
	_down->setProperty("themeID",
			   QVariant(QString::fromUtf8("downArrowIconSmall")));
	_down->setFlat(true);

	QWidget::connect(_address, SIGNAL(editingFinished()), this,
			 SLOT(AddressChanged()));
	QWidget::connect(_add, SIGNAL(clicked()), this, SLOT(Add()));
	QWidget::connect(_remove, SIGNAL(clicked()), this, SLOT(Remove()));
	QWidget::connect(_up, SIGNAL(clicked()), this, SLOT(Up()));
	QWidget::connect(_down, SIGNAL(clicked()), this, SLOT(Down()));

	auto controlsLayout = new QHBoxLayout();
	controlsLayout->addWidget(_add);
	controlsLayout->addWidget(_remove);
	QFrame *line = new QFrame();
	line->setFrameShape(QFrame::VLine);
	line->setFrameShadow(QFrame::Sunken);
	controlsLayout->addWidget(line);
	controlsLayout->addWidget(_up);
	controlsLayout->addWidget(_down);
	controlsLayout->addStretch();

	auto layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(_address);
	layout->addWidget(_elements);
	layout->addLayout(controlsLayout);
	setLayout(layout);
}

void OSCMessageEdit::InsertElement(const OSCMessageElement &element)
{
	auto item = new QListWidgetItem(_elements);
	_elements->addItem(item);
	auto elementEdit = new OSCMessageElementEdit(this);
	elementEdit->SetMessageElement(element);
	item->setSizeHint(elementEdit->minimumSizeHint());
	_elements->setItemWidget(item, elementEdit);
	QWidget::connect(elementEdit,
			 SIGNAL(ElementValueChanged(const OSCMessageElement &)),
			 this,
			 SLOT(ElementValueChanged(const OSCMessageElement &)));
	QWidget::connect(elementEdit, SIGNAL(Focussed()), this,
			 SLOT(ElementFocussed()));
	_currentSelection._elements.push_back(element);
}

void OSCMessageEdit::SetMessage(const OSCMessage &message)
{
	_address->setText(message._address);
	for (const auto &element : message._elements) {
		InsertElement(element);
	}
	_currentSelection = message;
	SetWidgetSize();
}

void OSCMessageEdit::AddressChanged()
{
	_currentSelection._address = _address->text().toStdString();
	emit MessageChanged(_currentSelection);
}

void OSCMessageEdit::Add()
{
	OSCMessageElement element;
	InsertElement(element);
	emit MessageChanged(_currentSelection);
	SetWidgetSize();
}

void OSCMessageEdit::Remove()
{
	auto item = _elements->currentItem();
	int idx = _elements->currentRow();
	if (!item || idx == -1) {
		return;
	}
	delete item;
	_currentSelection._elements.erase(_currentSelection._elements.begin() +
					  idx);
	emit MessageChanged(_currentSelection);
	SetWidgetSize();
}

void OSCMessageEdit::Up()
{
	int idx = _elements->currentRow();
	if (!listMoveUp(_elements)) {
		return;
	}

	iter_swap(_currentSelection._elements.begin() + idx,
		  _currentSelection._elements.begin() + idx - 1);

	emit MessageChanged(_currentSelection);
	SetWidgetSize();
}

void OSCMessageEdit::Down()
{
	int idx = _elements->currentRow();
	if (!listMoveDown(_elements)) {
		return;
	}

	iter_swap(_currentSelection._elements.begin() + idx,
		  _currentSelection._elements.begin() + idx + 1);

	emit MessageChanged(_currentSelection);
	SetWidgetSize();
}

static QListWidgetItem *getItemFromWidget(QListWidget *list, QWidget *widget)
{
	for (int i = 0; i < list->count(); i++) {
		auto item = list->item(i);
		if (!item) {
			continue;
		}
		auto itemWidget = list->itemWidget(item);
		if (itemWidget == widget) {
			return item;
		}
	}
	return nullptr;
}

int OSCMessageEdit::GetIndexOfSignal()
{
	auto sender = this->sender();
	if (!sender) {
		return -1;
	}
	auto widget = qobject_cast<QWidget *>(sender);
	if (!widget) {
		return -1;
	}
	return _elements->row(getItemFromWidget(_elements, widget));
}

void OSCMessageEdit::ElementFocussed()
{
	int idx = GetIndexOfSignal();
	if (idx == -1) {
		return;
	}
	_elements->setCurrentRow(idx);
}

void OSCMessageEdit::ElementValueChanged(const OSCMessageElement &element)
{
	int idx = GetIndexOfSignal();
	if (idx == -1) {
		return;
	}

	_currentSelection._elements.at(idx) = element;
	_elements->setCurrentRow(idx);
	emit MessageChanged(_currentSelection);
}

void OSCMessageEdit::SetWidgetSize()
{
	SetHeightToContentHeight(_elements);
	adjustSize();
	updateGeometry();
}

} // namespace advss
