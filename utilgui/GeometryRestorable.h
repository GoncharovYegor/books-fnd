#pragma once

#include "fnd/EnumBitmask.h"
#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

#include "settings/ISettings.h"

#include "export/utilgui.h"

class QScreen;
class QSplitter;

namespace HomeCompa::Util {

class UTILGUI_EXPORT GeometryRestorable
{
	NON_COPY_MOVABLE(GeometryRestorable)

public:
	enum class FontComponent
	{
		None   = 0,
		Family = 1 << 0,
		Size   = 1 << 1,
		All    = Family | Size
	};

	class IObserver // NOLINT(cppcoreguidelines-special-member-functions)
	{
	public:
		virtual ~IObserver()                  = default;
		virtual QWidget& GetWidget() noexcept = 0;

		virtual void OnFontChanged(const QFont&)
		{
		}
	};

protected:
	GeometryRestorable(IObserver& observer, std::shared_ptr<ISettings> settings, QString name);
	~GeometryRestorable();

protected:
	void LoadGeometry();
	void SaveGeometry();
	void SetFixedFont(QWidget* widget, FontComponent fontComponent);

private:
	class Impl;
	PropagateConstPtr<Impl> m_impl;
};

class UTILGUI_EXPORT GeometryRestorableObserver : public GeometryRestorable::IObserver
{
protected:
	explicit GeometryRestorableObserver(QWidget& widget);

protected: // GeometryRestorable::IObserver
	QWidget& GetWidget() noexcept override;

private:
	QWidget& m_widget;
};

UTILGUI_EXPORT void     MoveToParentCenter(QWidget& widget);
UTILGUI_EXPORT QScreen* GetActiveScreen(QWidget& widget);

} // namespace HomeCompa::Util

ENABLE_BITMASK_OPERATORS(HomeCompa::Util::GeometryRestorable::FontComponent);
