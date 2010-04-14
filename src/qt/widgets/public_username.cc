/*
 * copyright maidsafe.net limited 2009
 * The following source code is property of maidsafe.net limited and
 * is not meant for external use. The use of this code is governed
 * by the license file LICENSE.TXT found in the root of this directory and also
 * on www.maidsafe.net.
 *
 * You are not free to copy, amend or otherwise use this source code without
 * explicit written permission of the board rof directors of maidsafe.net
 *
 *  Created on: May 8, 2009
 *      Author: Team
 */

#include "qt/widgets/public_username.h"

// qt
#include <QMessageBox>

// core
#include "qt/client/client_controller.h"

// local
#include "qt/client/create_public_username_thread.h"

PublicUsername::PublicUsername(QWidget* parent)
    : Panel(parent)
    , init_(false) {
  ui_.setupUi(this);
  ui_.create->setAutoDefault(true);

  connect(ui_.create, SIGNAL(clicked(bool)),
          this,       SLOT(onCreateUsernameClicked()));

  connect(ui_.contactLineEdit, SIGNAL(returnPressed()),
          this,                SLOT(onCreateUsernameClicked()));

  ui_.progressLabel->setVisible(false);
  ui_.progressBar->setVisible(false);
}

void PublicUsername::clearPubUsername() {
  ui_.contactLineEdit->setText("");
}

void PublicUsername::setActive(bool b) {
  if (b && !init_) {
    init_ = true;
  }
}

void PublicUsername::reset() {
  init_ = false;
  ui_.progressBar->reset();
  ui_.progressLabel->setVisible(false);
  ui_.progressBar->setVisible(false);
}

PublicUsername::~PublicUsername() { }

void PublicUsername::onCreateUsernameClicked() {
  QString text = ui_.contactLineEdit->text().trimmed();
  if (text.isEmpty()) {
    QMessageBox::warning(this, tr("Problem!"),
                         tr("Please specify a Username."));
    return;
  }

  CreatePublicUsernameThread* cput = new CreatePublicUsernameThread(text, this);

  connect(cput, SIGNAL(completed(bool)),
          this, SLOT(onCreateUsernameCompleted(bool)));

  ui_.progressLabel->setVisible(true);
  ui_.progressBar->setVisible(true);

  cput->start();
}

void PublicUsername::onCreateUsernameCompleted(bool success) {
  if (success) {
    ui_.contactLineEdit->setText(tr(""));
    emit complete();
    ClientController::instance()->StartCheckingMessages();
  } else {
    QMessageBox::warning(this, tr("Problem!"), tr("Error setting Username."));
  }
  ui_.progressLabel->setVisible(false);
  ui_.progressBar->setVisible(false);
}
