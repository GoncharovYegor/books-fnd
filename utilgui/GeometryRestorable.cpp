#include "GeometryRestorable.h"

#include <QGuiApplication>
#include <QLayout>
#include <QMetaEnum>
#include <QScreen>
#include <QSplitter>
#include <QTimer>
#include <QWindow>

#include "fnd/algorithm.h"

#include "platformgui/PlatformGuiUtil.h"
#include "settings/ISettingsObserver.h"
#include "settings/UiTimer.h"

#include "Constant.h"
#include "Font.h"
#include "log.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace {

constexpr auto STATE_KEY_TEMPLATE    = "ui/%1/State";
constexpr auto GEOMETRY_KEY_TEMPLATE = "ui/%1/Geometry";
constexpr auto SPLITTER_KEY_TEMPLATE = "ui/%1/%2";

constexpr auto LAYOUT               = "Layout";
constexpr auto FRAME_LINE_WIDTH     = "lineWidth";
constexpr auto FRAME_MID_LINE_WIDTH = "midLineWidth";
constexpr auto FRAME_SHAPE          = "frameShape";
constexpr auto FRAME_SHADOW         = "frameShadow";

void InitSplitter(QSplitter* splitter)
{
	QTimer::singleShot(0, splitter, [splitter] {
		QList<int> sizes = splitter->sizes();
		const auto width = std::accumulate(sizes.cbegin(), sizes.cend(), 0);

		QList<int> stretch;
		if (splitter->orientation() == Qt::Horizontal)
			for (int i = 0, sz = splitter->count(); i < sz; ++i)
				stretch << splitter->widget(i)->sizePolicy().horizontalStretch();
		else
			for (int i = 0, sz = splitter->count(); i < sz; ++i)
				stretch << splitter->widget(i)->sizePolicy().verticalStretch();

		const auto weightSum = std::accumulate(stretch.cbegin(), stretch.cend(), 0);
		if (weightSum == 0)
			return;

		std::ranges::transform(stretch, sizes.begin(), [=](const auto weight) {
			return width * weight / weightSum;
		});

		splitter->setSizes(sizes);
	});
}

void SetGeometry(QWidget& widget, QRect rect)
{
	if (std::ranges::none_of(QGuiApplication::screens(), [center = rect.center()](const auto* screen) {
			return screen->availableGeometry().contains(center);
		}))
		rect.moveCenter(QGuiApplication::primaryScreen()->availableGeometry().center());

	Platform::SetGeometry(widget, rect);
}

QString GetSerializeProcessKey(const QString& name, const QString& key)
{
	return QString("UiCustomization%1/%2").arg(name).arg(key);
}

} // namespace

class GeometryRestorable::Impl final
    : QObject
    , ISettingsObserver
{
	NON_COPY_MOVABLE(Impl)

public:
	Impl(IObserver& observer, std::shared_ptr<ISettings> settings, QString name)
		: m_observer(observer)
		, m_settings(std::move(settings))
		, m_name(std::move(name))
	{
		m_settings->RegisterObserver(this);
	}

	~Impl() override
	{
		assert(!m_initialized);
	}

	void LoadGeometry()
	{
		m_observer.GetWidget().installEventFilter(this);
		auto uniqueCheck = [
#ifndef NDEBUG
							   uniqueList = std::unordered_set<QString> {}
#endif
		](const QString& name) mutable -> const QString& {
#ifndef NDEBUG
			assert(uniqueList.insert(name).second && "names must be unique");
#endif
			return name;
		};

		for (auto* splitter : m_observer.GetWidget().findChildren<QSplitter*>())
		{
			if (const auto value = m_settings->Get(QString(SPLITTER_KEY_TEMPLATE).arg(m_name).arg(uniqueCheck(splitter->objectName()))); value.isValid())
				splitter->restoreState(value.toByteArray());
			else
				InitSplitter(splitter);
			splitter->setChildrenCollapsible(false);
		}
	}

	void LoadLayout()
	{
		SerializationProcess(m_observer.GetWidget(), {}, &Impl::DeserializeWidget);
	}

	void SaveLayout()
	{
		m_settings->Remove("UiCustomization");
		SerializationProcess(m_observer.GetWidget(), {}, &Impl::SerializeWidget);
	}

	void OnHide()
	{
		if (!Set(m_initialized, false))
			return;

		const auto state = m_observer.GetWidget().windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen);
		m_settings->Set(QString(STATE_KEY_TEMPLATE).arg(m_name), static_cast<int>(state));

		if ((state & (Qt::WindowMaximized | Qt::WindowFullScreen)) == Qt::WindowNoState)
			m_settings->Set(QString(GEOMETRY_KEY_TEMPLATE).arg(m_name), m_observer.GetWidget().geometry());

		for (const auto* splitter : m_observer.GetWidget().findChildren<QSplitter*>())
			m_settings->Set(QString(SPLITTER_KEY_TEMPLATE).arg(m_name).arg(splitter->objectName()), splitter->saveState());
	}

	void OnShow()
	{
		if (!Set(m_initialized, true))
			return;

		OnFontChanged();

		if (const auto state = m_observer.GetWidget().windowState(); state & (Qt::WindowMaximized | Qt::WindowFullScreen))
			return;

		if (const auto stateVar = m_settings->Get(QString(GEOMETRY_KEY_TEMPLATE).arg(m_name)); stateVar.isValid())
			SetGeometry(m_observer.GetWidget(), stateVar.toRect());

		if (const auto value = m_settings->Get(QString(STATE_KEY_TEMPLATE).arg(m_name)).toInt(); value & (Qt::WindowMaximized | Qt::WindowFullScreen))
			m_observer.GetWidget().setWindowState(static_cast<Qt::WindowState>(value & ~Qt::WindowMinimized));
	}

	void SetFixedFont(QWidget* widget, const FontComponent fontComponent)
	{
		m_fontFixed[widget] = fontComponent;
	}

private: // QObject
	bool eventFilter(QObject* target, QEvent* event) override
	{
		if (event->type() == QEvent::Show)
			OnShow();

		if (event->type() == QEvent::Hide)
			OnHide();

		return m_observer.GetWidget().eventFilter(target, event);
	}

private: // ISettingsObserver
	void HandleValueChanged(const QString& key, const QVariant&) override
	{
		if (key.startsWith(Global::FONT_KEY))
			m_fontTimer->start();
	}

private:
	void OnFontChanged()
	{
		const SettingsGroup group(*m_settings, Global::FONT_KEY);
		auto                font = m_observer.GetWidget().font();
		Deserialize(font, *m_settings);
		const auto sizeFactor = font.pointSizeF() / Global::FONT_SIZE_DEFAULT;

		for (auto& [widget, pointSize] : CollectWidgets())
		{
			const auto it             = m_fontFixed.find(widget);
			const auto fixedComponent = it != m_fontFixed.end() ? it->second : FontComponent::None;
			auto       f              = widget->font();
			if (!(fixedComponent & FontComponent::Size))
				f.setPointSizeF(pointSize * sizeFactor);
			if (!(fixedComponent & FontComponent::Family))
				f.setFamily(font.family());
			widget->setFont(f);
		}

		m_observer.OnFontChanged(font);
	}

	static void EnumerateWidgets(QWidget& parent, const std::function<void(QWidget&)>& f)
	{
		f(parent);
		for (auto* widget : parent.findChildren<QWidget*>(QString {}, Qt::FindDirectChildrenOnly))
			EnumerateWidgets(*widget, f);
	}

	std::vector<std::pair<QWidget*, qreal>>& CollectWidgets()
	{
		if (m_widgets.empty())
			EnumerateWidgets(m_observer.GetWidget(), [&](QWidget& widget) {
				m_widgets.emplace_back(&widget, widget.font().pointSizeF());
			});
		return m_widgets;
	}

	void SerializationProcess(QWidget& widget, const QString& objectName, void (Impl::*f)(QWidget&, const QString&))
	{
		auto parentName = objectName;
		if (const auto widgetObjectName = widget.objectName(); !widgetObjectName.isEmpty())
		{
			parentName.append("/").append(widgetObjectName);
			std::invoke(f, this, widget, parentName);
		}
		for (auto* child : widget.findChildren<QWidget*>(QString {}, Qt::FindDirectChildrenOnly))
			SerializationProcess(*child, parentName, f);
	}

	void SerializeWidget(QWidget& widget, const QString& name)
	{
		if (const auto* layout = widget.layout())
			SerializeLayout(*layout, name);

		if (const auto* frame = qobject_cast<const QFrame*>(&widget))
			SerializeFrame(*frame, name);
	}

	void SerializeLayout(const QLayout& layout, const QString& name)
	{
		const auto m = layout.contentsMargins();
		m_settings->Set(GetSerializeProcessKey(name, LAYOUT), QString("%1,%2,%3,%4,%5").arg(m.left()).arg(m.right()).arg(m.top()).arg(m.bottom()).arg(layout.spacing()));
	}

	void SerializeFrame(const QFrame& frame, const QString& name)
	{
		m_settings->Set(GetSerializeProcessKey(name, FRAME_LINE_WIDTH), frame.lineWidth());
		m_settings->Set(GetSerializeProcessKey(name, FRAME_MID_LINE_WIDTH), frame.midLineWidth());
		m_settings->Set(GetSerializeProcessKey(name, FRAME_SHAPE), QMetaEnum::fromType<QFrame::Shape>().valueToKey(frame.frameShape()));
		m_settings->Set(GetSerializeProcessKey(name, FRAME_SHADOW), QMetaEnum::fromType<QFrame::Shadow>().valueToKey(frame.frameShadow()));
	}

	void DeserializeWidget(QWidget& widget, const QString& name)
	{
		if (auto* layout = widget.layout())
			DeserializeLayout(*layout, name);

		if (auto* frame = qobject_cast<QFrame*>(&widget))
			DeserializeFrame(*frame, name);
	}

	void DeserializeLayout(QLayout& layout, const QString& name)
	{
		const auto layoutVar = m_settings->Get(GetSerializeProcessKey(name, LAYOUT));
		if (!layoutVar.isValid())
			return;

		const auto values = layoutVar.toString().split(',', Qt::KeepEmptyParts);

		const auto toInt = [&](const int n) -> std::optional<int> {
			if (n >= values.size())
				return std::nullopt;
			bool ok = false;
			if (const auto value = values[n].toInt(&ok); ok)
				return value;
			return std::nullopt;
		};

		auto m = layout.contentsMargins();

		if (const auto value = toInt(0))
			m.setLeft(*value);
		if (const auto value = toInt(1))
			m.setRight(*value);
		if (const auto value = toInt(2))
			m.setTop(*value);
		if (const auto value = toInt(3))
			m.setBottom(*value);
		if (const auto value = toInt(4))
			layout.setSpacing(*value);

		layout.setContentsMargins(m);
	}

	void DeserializeFrame(QFrame& frame, const QString& name)
	{
		const auto getInt = [&](const char* key) -> std::optional<int> {
			const auto var = m_settings->Get(GetSerializeProcessKey(name, key));
			if (!var.isValid())
				return std::nullopt;

			bool ok = false;
			if (const auto value = var.toInt(&ok); ok)
				return value;

			return std::nullopt;
		};

		auto getEnum = [&]<typename EnumType>(const char* key) -> std::optional<EnumType> {
			const auto var = m_settings->Get(GetSerializeProcessKey(name, key));
			if (!var.isValid())
				return std::nullopt;

			const QMetaEnum metaEnum = QMetaEnum::fromType<EnumType>();
			if (!metaEnum.isValid())
			{
				assert(false && "unexpected type");
				return std::nullopt;
			}

			const int value = metaEnum.keyToValue(var.toByteArray().constData());
			return value >= -1 ? std::optional { static_cast<EnumType>(value) } : std::nullopt;
		};

		if (const auto value = getInt(FRAME_LINE_WIDTH))
			frame.setLineWidth(*value);
		if (const auto value = getInt(FRAME_MID_LINE_WIDTH))
			frame.setMidLineWidth(*value);
		if (const auto value = getEnum.operator()<QFrame::Shape>(FRAME_SHAPE))
			frame.setFrameShape(*value);
		if (const auto value = getEnum.operator()<QFrame::Shadow>(FRAME_SHADOW))
			frame.setFrameShadow(*value);
	}

private:
	IObserver&                                    m_observer;
	PropagateConstPtr<ISettings, std::shared_ptr> m_settings;
	const QString                                 m_name;
	bool                                          m_initialized { false };
	std::vector<std::pair<QWidget*, qreal>>       m_widgets;
	std::unordered_map<QWidget*, FontComponent>   m_fontFixed;

	PropagateConstPtr<QTimer> m_fontTimer { CreateUiTimer([&] {
		OnFontChanged();
	}) };
};

GeometryRestorable::GeometryRestorable(IObserver& observer, std::shared_ptr<ISettings> settings, QString name)
	: m_impl(observer, std::move(settings), std::move(name))
{
}

GeometryRestorable::~GeometryRestorable() = default;

void GeometryRestorable::LoadGeometry()
{
	m_impl->LoadGeometry();
}

void GeometryRestorable::SaveGeometry()
{
	m_impl->OnHide();
}

void GeometryRestorable::LoadLayout()
{
	m_impl->LoadLayout();
}

void GeometryRestorable::SaveLayout()
{
	m_impl->SaveLayout();
}

void GeometryRestorable::SetFixedFont(QWidget* widget, const FontComponent fontComponent)
{
	m_impl->SetFixedFont(widget, fontComponent);
}

GeometryRestorableObserver::GeometryRestorableObserver(QWidget& widget)
	: m_widget(widget)
{
}

QWidget& GeometryRestorableObserver::GetWidget() noexcept
{
	return m_widget;
}

namespace HomeCompa::Util {

void MoveToParentCenter(QWidget& widget)
{
	auto* parent = widget.parentWidget();
	if (!parent)
		return;

	auto       rect         = widget.geometry();
	const auto parentCenter = parent->geometry().center();
	rect.translate(parentCenter - rect.center());
	Platform::SetGeometry(widget, rect);
}

QScreen* GetActiveScreen(QWidget& widget)
{
	for (auto* pWidget = &widget; pWidget; pWidget = pWidget->parentWidget())
		if (auto* w = pWidget->windowHandle())
			return w->screen();

	return nullptr;
}

} // namespace HomeCompa::Util
