#include "achievementsettingswidget.h"
#include "achievementlogindialog.h"
#include "common/string_util.h"
#include "core/system.h"
#include "frontend-common/cheevos.h"
#include "qtutils.h"
#include "settingsdialog.h"
#include "settingwidgetbinder.h"
#include <QtCore/QDateTime>
#include <QtWidgets/QMessageBox>

AchievementSettingsWidget::AchievementSettingsWidget(QtHostInterface* host_interface, QWidget* parent,
                                                     SettingsDialog* dialog)
  : QWidget(parent), m_host_interface(host_interface)
{
  m_ui.setupUi(this);

  SettingWidgetBinder::BindWidgetToBoolSetting(m_host_interface, m_ui.enable, "Cheevos", "Enabled", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_host_interface, m_ui.richPresence, "Cheevos", "RichPresence", true);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_host_interface, m_ui.testMode, "Cheevos", "TestMode", false);
  SettingWidgetBinder::BindWidgetToBoolSetting(m_host_interface, m_ui.useFirstDiscFromPlaylist, "Cheevos",
                                               "UseFirstDiscFromPlaylist", true);

  dialog->registerWidgetHelp(m_ui.enable, tr("Enable Achievements"), tr("Unchecked"),
                             tr("When enabled and logged in, DuckStation will scan for achievements on startup."));
  dialog->registerWidgetHelp(m_ui.testMode, tr("Enable Test Mode"), tr("Unchecked"),
                             tr("When enabled, DuckStation will assume all achievements are locked and not send any "
                                "unlock notifications to the server."));
  dialog->registerWidgetHelp(
    m_ui.richPresence, tr("Enable Rich Presence"), tr("Unchecked"),
    tr("When enabled, rich presence information will be collected and sent to the server where supported."));
  dialog->registerWidgetHelp(
    m_ui.useFirstDiscFromPlaylist, tr("Use First Disc From Playlist"), tr("Unchecked"),
    tr(
      "When enabled, the first disc in a playlist will be used for achievements, regardless of which disc is active."));

  connect(m_ui.enable, &QCheckBox::stateChanged, this, &AchievementSettingsWidget::updateEnableState);
  connect(m_ui.loginButton, &QPushButton::clicked, this, &AchievementSettingsWidget::onLoginLogoutPressed);
  connect(m_ui.viewProfile, &QPushButton::clicked, this, &AchievementSettingsWidget::onViewProfilePressed);
  connect(host_interface, &QtHostInterface::achievementsLoaded, this, &AchievementSettingsWidget::onAchievementsLoaded);

  updateEnableState();
  updateLoginState();

  // force a refresh of game info
  host_interface->OnAchievementsRefreshed();
}

AchievementSettingsWidget::~AchievementSettingsWidget() = default;

void AchievementSettingsWidget::updateEnableState()
{
  const bool enabled = m_host_interface->GetBoolSettingValue("Cheevos", "Enabled", false);
  m_ui.testMode->setEnabled(enabled);
  m_ui.useFirstDiscFromPlaylist->setEnabled(enabled);
  m_ui.richPresence->setEnabled(enabled);
}

void AchievementSettingsWidget::updateLoginState()
{
  const std::string username(m_host_interface->GetStringSettingValue("Cheevos", "Username"));
  const bool logged_in = !username.empty();

  if (logged_in)
  {
    const u64 login_unix_timestamp =
      StringUtil::FromChars<u64>(m_host_interface->GetStringSettingValue("Cheevos", "LoginTimestamp", "0")).value_or(0);
    const QDateTime login_timestamp(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(login_unix_timestamp)));
    m_ui.loginStatus->setText(tr("Username: %1\nLogin token generated on %2.")
                                .arg(QString::fromStdString(username))
                                .arg(login_timestamp.toString(Qt::TextDate)));
    m_ui.loginButton->setText(tr("Logout"));
  }
  else
  {
    m_ui.loginStatus->setText(tr("Not Logged In."));
    m_ui.loginButton->setText(tr("Login..."));
    m_ui.viewProfile->setEnabled(false);
  }
}

void AchievementSettingsWidget::onLoginLogoutPressed()
{
  if (!m_host_interface->GetStringSettingValue("Cheevos", "Username").empty())
  {
    m_host_interface->executeOnEmulationThread([]() { Cheevos::Logout(); }, true);
    updateLoginState();
    return;
  }

  AchievementLoginDialog login(this);
  int res = login.exec();
  if (res != 0)
    return;

  updateLoginState();
}

void AchievementSettingsWidget::onViewProfilePressed()
{
  if (!Cheevos::IsLoggedIn())
    return;

  const QByteArray encoded_username(QUrl::toPercentEncoding(QString::fromStdString(Cheevos::GetUsername())));
  QtUtils::OpenURL(
    QtUtils::GetRootWidget(this),
    QUrl(QStringLiteral("https://retroachievements.org/user/%1").arg(QString::fromUtf8(encoded_username))));
}

void AchievementSettingsWidget::onAchievementsLoaded(quint32 id, const QString& game_info_string, quint32 total,
                                                     quint32 points)
{
  m_ui.gameInfo->setText(game_info_string);
}