/*
* ============================================================================
*
* Copyright [2009] maidsafe.net limited
*
* Description:  Handles user's list of maidsafe private shares
* Version:      1.0
* Created:      2009-01-28-23.19.56
* Revision:     none
* Compiler:     gcc
* Author:       Fraser Hutchison (fh)
*               alias "The Hutch"
*               fraser.hutchison@maidsafe.net
* Company:      maidsafe.net limited
*
* The following source code is property of maidsafe.net limited and is not
* meant for external use.  The use of this code is governed by the license
* file LICENSE.TXT found in the root of this directory and also on
* www.maidsafe.net.
*
* You are not free to copy, amend or otherwise use this source code without
* the explicit written permission of the board of directors of maidsafe.net.
*
* ============================================================================
*/

#include <boost/filesystem.hpp>
#include <gtest/gtest.h>
#include <maidsafe/utils.h>
#include "maidsafe/client/privateshares.h"

void RefillParticipants(std::list<maidsafe::ShareParticipants> *participants) {
  participants->clear();
  maidsafe::ShareParticipants r;
  r.id = "Dan";
  r.public_key = base::RandomString(512);
  r.role = 'R';
  participants->push_back(r);
  r.id = "The Hutch";
  r.public_key = base::RandomString(512);
  r.role = 'A';
  participants->push_back(r);
}

class PrivateSharesTest : public testing::Test {
  protected:
    maidsafe::PrivateShareHandler *psh_;
    maidsafe::PrivateShare *ps_;
    std::string name;
    std::list<maidsafe::ShareParticipants> participants;
    std::vector<std::string> attributes;

    PrivateSharesTest() : psh_(NULL), ps_(NULL),
      name(), participants(),
      attributes() {
    }
    PrivateSharesTest(const PrivateSharesTest&);
    PrivateSharesTest& operator=(const PrivateSharesTest&) {
      return *this;
    }

    virtual void SetUp() {
      psh_ = new maidsafe::PrivateShareHandler();
      std::string share_name("My First Share");
      attributes.push_back(share_name);
      attributes.push_back(base::RandomString(64));
      attributes.push_back(base::RandomString(512));
      attributes.push_back(base::RandomString(512));
      maidsafe::ShareParticipants r;
      r.id = "Dan";
      r.public_key = base::RandomString(512);
      r.role = 'R';
      participants.push_back(r);
      r.id = "The Hutch";
      r.public_key = base::RandomString(512);
      r.role = 'A';
      participants.push_back(r);
      ps_ = new maidsafe::PrivateShare(attributes, participants);
    }

    virtual void TearDown() {
      delete psh_;
      delete ps_;
    }
};

TEST_F(PrivateSharesTest, BEH_MAID_MI_Create_ListShares) {
  // Test share list to be empty
  std::list<maidsafe::private_share> share_list;
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(0), share_list.size()) <<
            "Share container not empty on creation.";

  // Test full share list to be empty
  std::list<maidsafe::PrivateShare> full_share_list;
  ASSERT_EQ(0, psh_->MI_GetFullShareList(&full_share_list)) <<
            "Failed getting full list";
  ASSERT_EQ(static_cast<unsigned int>(0), full_share_list.size()) <<
            "Share container not empty on creation.";

  // Test lower bound of field index
  maidsafe::PrivateShare ps;
  ASSERT_EQ(-2014, psh_->MI_GetShareInfo("aaa", -1, &ps)) <<
            "Failed to recognise invalid field (-1).";
  ASSERT_EQ("", ps.Name()) << "Share container Name was modified.";
  ASSERT_EQ("", ps.Msid()) << "Share container Msid was modified.";
  ASSERT_EQ("", ps.MsidPubKey()) << "Share container MsidPubKey was modified.";
  ASSERT_EQ("", ps.MsidPriKey()) << "Share container MsidPriKey was modified.";
  ASSERT_EQ(static_cast<unsigned int>(0), ps.Participants().size()) <<
            "Share container Participants was modified.";

  // Test upper bound of field index
  ASSERT_EQ(-2014, psh_->MI_GetShareInfo("aaa", 2, &ps)) <<
            "Failed to recognise invalid field (2).";
  ASSERT_EQ("", ps.Name()) << "Share container Name was modified.";
  ASSERT_EQ("", ps.Msid()) << "Share container Msid was modified.";
  ASSERT_EQ("", ps.MsidPubKey()) << "Share container MsidPubKey was modified.";
  ASSERT_EQ("", ps.MsidPriKey()) << "Share container MsidPriKey was modified.";
  ASSERT_EQ(static_cast<unsigned int>(0), ps.Participants().size()) <<
            "Share container Participants was modified.";

  // Test wrong share name
  ASSERT_EQ(-2014, psh_->MI_GetShareInfo("aaa", 0, &ps)) <<
            "Failed to recognise invalid share name.";
  ASSERT_EQ("", ps.Name()) << "Share container Name was modified.";
  ASSERT_EQ("", ps.Msid()) << "Share container Msid was modified.";
  ASSERT_EQ("", ps.MsidPubKey()) << "Share container MsidPubKey was modified.";
  ASSERT_EQ("", ps.MsidPriKey()) << "Share container MsidPriKey was modified.";
  ASSERT_EQ(static_cast<unsigned int>(0), ps.Participants().size()) <<
            "Share container Participants was modified.";

  // Test wrong share msid
  ASSERT_EQ(-2014, psh_->MI_GetShareInfo("aaa", 1, &ps)) <<
            "Failed to recognise invalid share msid.";
  ASSERT_EQ("", ps.Name()) << "Share container Name was modified.";
  ASSERT_EQ("", ps.Msid()) << "Share container Msid was modified.";
  ASSERT_EQ("", ps.MsidPubKey()) << "Share container MsidPubKey was modified.";
  ASSERT_EQ("", ps.MsidPriKey()) << "Share container MsidPriKey was modified.";
  ASSERT_EQ(static_cast<unsigned int>(0), ps.Participants().size()) <<
            "Share container Participants was modified.";

  // Test wrong index for field in share participant lookup
  std::list<maidsafe::share_participant> sp_list;
  ASSERT_EQ(-2015, psh_->MI_GetParticipantsList("aaa", -1, &sp_list)) <<
            "Failed to recognise lower bound.";
  ASSERT_EQ(static_cast<unsigned int>(0), sp_list.size()) <<
            "List should have remained empty.";
  ASSERT_EQ(-2015, psh_->MI_GetParticipantsList("aaa", 2, &sp_list)) <<
            "Failed to recognise upper bound.";
  ASSERT_EQ(static_cast<unsigned int>(0), sp_list.size()) <<
            "List should have remained empty.";

  // Test wrong share name for participant list
  ASSERT_EQ(-2015, psh_->MI_GetParticipantsList("aaa", 0, &sp_list)) <<
            "Failed to recognise invalid share name.";
  ASSERT_EQ(static_cast<unsigned int>(0), sp_list.size()) <<
            "List should have remained empty.";

  // Test wrong share msid for participant list
  ASSERT_EQ(-2015, psh_->MI_GetParticipantsList("aaa", 1, &sp_list)) <<
            "Failed to recognise invalid share msid.";
  ASSERT_EQ(static_cast<unsigned int>(0), sp_list.size()) <<
            "List should have remained empty.";
}

TEST_F(PrivateSharesTest, BEH_MAID_MI_AddShare) {
  // Test share list to be empty
  std::list<maidsafe::private_share> share_list;
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(0), share_list.size()) <<
            "Share container not empty on creation.";

  // Copy the list for comparison
  std::list<maidsafe::ShareParticipants> cp = participants;

  // Add private share
  ASSERT_EQ(0, psh_->MI_AddPrivateShare(attributes, &cp)) <<
            "Failed to add share";

  // Check with GetShareInfo
  maidsafe::PrivateShare by_name;
  ASSERT_EQ(0, psh_->MI_GetShareInfo(attributes[0], 0, &by_name)) <<
            "Failed to locate share by name";
  ASSERT_EQ(attributes[0], by_name.Name()) << "Name different.";
  ASSERT_EQ(attributes[1], by_name.Msid()) << "Msid different.";
  ASSERT_EQ(attributes[2], by_name.MsidPubKey()) << "MsidPubKey different.";
  ASSERT_EQ(attributes[3], by_name.MsidPriKey()) << "MsidPriKey different.";
  ASSERT_EQ(participants.size(), by_name.Participants().size()) <<
            "Participant lists different in size.";
  maidsafe::PrivateShare by_msid;
  ASSERT_EQ(0, psh_->MI_GetShareInfo(attributes[1], 1, &by_msid)) <<
            "Failed to locate share by msid";
  ASSERT_EQ(attributes[0], by_msid.Name()) << "Name different.";
  ASSERT_EQ(attributes[1], by_msid.Msid()) << "Msid different.";
  ASSERT_EQ(attributes[2], by_msid.MsidPubKey()) << "MsidPubKey different.";
  ASSERT_EQ(attributes[3], by_msid.MsidPriKey()) << "MsidPriKey different.";
  ASSERT_EQ(participants.size(), by_msid.Participants().size()) <<
            "Participant lists different in size.";

  // Check with GetShareList
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(1), share_list.size()) <<
            "Share container empty on selection.";
  ASSERT_EQ(attributes[0], share_list.front().name_) << "Name different.";
  ASSERT_EQ(attributes[1], share_list.front().msid_) << "Msid different.";
  ASSERT_EQ(attributes[2], share_list.front().msid_pub_key_) <<
            "MsidPubKey different.";
  ASSERT_EQ(attributes[3], share_list.front().msid_priv_key_) <<
            "MsidPriKey different.";

  // Check Participants with share name
  std::list<maidsafe::share_participant> sp_list;
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[0], 0, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(2), participants.size()) <<
            "Probably forgot to refill original list.";
  ASSERT_EQ(participants.size(), sp_list.size()) <<
            "Retrived list size does not match.";
  maidsafe::share_participant sp1 = sp_list.front();
  sp_list.pop_front();
  maidsafe::share_participant sp2 = sp_list.front();
  sp_list.pop_front();
  ASSERT_EQ(static_cast<unsigned int>(0), sp_list.size()) <<
            "Retrived list not empty.";

  for (std::list<maidsafe::ShareParticipants>::iterator it =
       participants.begin(); it != participants.end(); it++) {
    ASSERT_TRUE((*it).id == sp1.public_name_ || (*it).id == sp2.public_name_) <<
                "This element doesn't match one of the expected elements.";
    ASSERT_TRUE((*it).public_key == sp1.public_key_ ||
                (*it).public_key == sp2.public_key_) <<
                "This element doesn't match one of the expected elements.";
    ASSERT_TRUE((*it).role == sp1.role_ || (*it).role == sp2.role_) <<
                "This element doesn't match one of the expected elements.";
  }

  // Check Participants with share msid
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[1], 1, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(2), participants.size()) <<
            "Probably forgot to refill original list.";
  ASSERT_EQ(participants.size(), sp_list.size()) <<
            "Retrived list size does not match.";
  sp1 = sp_list.front();
  sp_list.pop_front();
  sp2 = sp_list.front();
  sp_list.pop_front();
  ASSERT_EQ(static_cast<unsigned int>(0), sp_list.size()) <<
            "Retrived list not empty.";

  for (std::list<maidsafe::ShareParticipants>::iterator it =
       participants.begin(); it != participants.end(); it++) {
    ASSERT_TRUE((*it).id == sp1.public_name_ || (*it).id == sp2.public_name_) <<
                "This element doesn't match one of the expected elements.";
    ASSERT_TRUE((*it).public_key == sp1.public_key_ ||
                (*it).public_key == sp2.public_key_) <<
                "This element doesn't match one of the expected elements.";
    ASSERT_TRUE((*it).role == sp1.role_ || (*it).role == sp2.role_) <<
                "This element doesn't match one of the expected elements.";
  }

  cp = participants;

  // Add same private share again
  ASSERT_EQ(0, psh_->MI_AddPrivateShare(attributes, &cp)) <<
            "Failed to add share";
  // Check with GetShareList
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(1), share_list.size()) <<
            "Share container empty or with > 1 element.";
}

TEST_F(PrivateSharesTest, BEH_MAID_MI_AddMultipleShares) {
  // Test share list to be empty
  std::list<maidsafe::private_share> share_list;
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(0), share_list.size()) <<
            "Share container not empty on creation.";

  // Copy the list for comparison
  std::list<maidsafe::ShareParticipants> cp;

  for (int n = 0; n < 10; n++) {
    // Attributes
    std::vector<std::string> atts;
    atts.push_back("NAME_" + base::itos(n));
    atts.push_back("MSID_" + base::itos(n));
    atts.push_back("MSID_PUB_KEY_" + base::itos(n));
    atts.push_back("MSID_PRI_KEY_" + base::itos(n));

    // Participants
    cp.clear();
    for (int a = 0; a < n + 1; a++) {
      maidsafe::ShareParticipants sps;
      sps.id = "PUB_NAME_" + base::itos(n) + "_" + base::itos(a);
      sps.public_key = "PUB_NAME_PUB_KEY_" + base::itos(n) +
                       "_" + base::itos(a);
      sps.role = 'C';
      cp.push_back(sps);
    }

    // Add private share
    ASSERT_EQ(0, psh_->MI_AddPrivateShare(atts, &cp)) <<
              "Failed to add share";
  }

  // Test full share list
  std::list<maidsafe::PrivateShare> full_share_list;
  ASSERT_EQ(0, psh_->MI_GetFullShareList(&full_share_list)) <<
            "Failed getting full list";
  ASSERT_EQ(static_cast<unsigned int>(10), full_share_list.size()) <<
            "Share container not empty on creation.";
  int y = 0;
  while (!full_share_list.empty()) {
    int e = 0;
    maidsafe::PrivateShare ps = full_share_list.front();
    full_share_list.pop_front();
    ASSERT_EQ("NAME_" + base::itos(y), ps.Name()) << "Name not equal.";
    ASSERT_EQ("MSID_" + base::itos(y), ps.Msid()) << "Msid not equal.";
    ASSERT_EQ("MSID_PUB_KEY_" + base::itos(y), ps.MsidPubKey()) <<
              "MsidPubKey not equal.";
    ASSERT_EQ("MSID_PRI_KEY_" + base::itos(y), ps.MsidPriKey()) <<
              "MsidPriKey not equal.";
    std::list<maidsafe::ShareParticipants> sps = ps.Participants();
    ASSERT_EQ(static_cast<unsigned int>(y + 1), sps.size()) <<
              "Participants number not the one expected.";
    while (!sps.empty()) {
      maidsafe::ShareParticipants sp = sps.front();
      sps.pop_front();
      ASSERT_EQ("PUB_NAME_" + base::itos(y) + "_" + base::itos(e), sp.id) <<
                "Wrong public name.";
      ASSERT_EQ("PUB_NAME_PUB_KEY_" + base::itos(y) + "_" + base::itos(e),
                sp.public_key) << "Wrong public name key.";
      ASSERT_EQ('C', sp.role) << "Wrong role.";
      e++;
    }
    y++;
  }

  unsigned int l = base::random_32bit_uinteger() % 10;
  maidsafe::PrivateShare by_name;
  ASSERT_EQ(0, psh_->MI_GetShareInfo("NAME_" + base::itos(l), 0, &by_name)) <<
            "Failed to locate share by name";
  ASSERT_EQ("NAME_" + base::itos(l), by_name.Name()) << "Name different.";
  ASSERT_EQ("MSID_" + base::itos(l), by_name.Msid()) << "Msid different.";
  ASSERT_EQ("MSID_PUB_KEY_" + base::itos(l), by_name.MsidPubKey()) <<
            "MsidPubKey different.";
  ASSERT_EQ("MSID_PRI_KEY_" + base::itos(l), by_name.MsidPriKey()) <<
            "MsidPriKey different.";
  ASSERT_EQ(static_cast<unsigned int>(l + 1), by_name.Participants().size()) <<
            "Participant lists different in size.";
  std::list<maidsafe::ShareParticipants> sps = by_name.Participants();
  int i = 0;
  while (!sps.empty()) {
    maidsafe::ShareParticipants sp = sps.front();
    sps.pop_front();
    ASSERT_EQ("PUB_NAME_" + base::itos(l) + "_" + base::itos(i), sp.id) <<
              "Wrong public name.";
    ASSERT_EQ("PUB_NAME_PUB_KEY_" + base::itos(l) + "_" + base::itos(i),
              sp.public_key) << "Wrong public name key.";
    ASSERT_EQ('C', sp.role) << "Wrong role.";
    i++;
  }
}

TEST_F(PrivateSharesTest, BEH_MAID_MI_DeleteShare) {
  // Test share list to be empty
  std::list<maidsafe::private_share> share_list;
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(0), share_list.size()) <<
            "Share container not empty on creation.";

  // Copy the list for comparison
  std::list<maidsafe::ShareParticipants> cp;

  for (int n = 0; n < 10; n++) {
    // Attributes
    std::vector<std::string> atts;
    atts.push_back("NAME_" + base::itos(n));
    atts.push_back("MSID_" + base::itos(n));
    atts.push_back("MSID_PUB_KEY_" + base::itos(n));
    atts.push_back("MSID_PRI_KEY_" + base::itos(n));

    // Participants
    cp.clear();
    for (int a = 0; a < n + 1; a++) {
      maidsafe::ShareParticipants sps;
      sps.id = "PUB_NAME_" + base::itos(n) + "_" + base::itos(a);
      sps.public_key = "PUB_NAME_PUB_KEY_" + base::itos(n) +
                       "_" + base::itos(a);
      sps.role = 'C';
      cp.push_back(sps);
    }

    // Add private share
    ASSERT_EQ(0, psh_->MI_AddPrivateShare(atts, &cp)) <<
              "Failed to add share";
  }

  // Test full share list
  std::list<maidsafe::PrivateShare> full_share_list;
  ASSERT_EQ(0, psh_->MI_GetFullShareList(&full_share_list)) <<
            "Failed getting full list";
  ASSERT_EQ(static_cast<unsigned int>(10), full_share_list.size()) <<
            "Share container not empty on creation.";

  // Delete random share by name
  unsigned int l = base::random_32bit_uinteger() % 10;
  ASSERT_EQ(0, psh_->MI_DeletePrivateShare("NAME_" + base::itos(l), 0)) <<
            "Failed to delete.";

  // Full share list
  ASSERT_EQ(0, psh_->MI_GetFullShareList(&full_share_list)) <<
            "Failed getting full list";
  ASSERT_EQ(static_cast<unsigned int>(9), full_share_list.size()) <<
            "Share container not empty on creation.";

  maidsafe::PrivateShare by_name, by_msid;
  std::list<maidsafe::share_participant> sp_list;
  // Find by share name
  ASSERT_EQ(-2014, psh_->MI_GetShareInfo("NAME_" + base::itos(l), 0, &by_name))
            << "Located share by name.";
  // Find by share msid
  ASSERT_EQ(-2014, psh_->MI_GetShareInfo("MSID_" + base::itos(l), 1, &by_msid))
            << "Located share by msid.";
  // Find the participants of the share
  ASSERT_EQ(-2015, psh_->MI_GetParticipantsList("MSID_" + base::itos(l), 1,
            &sp_list)) << "Some participants of the share remain.";
  ASSERT_EQ(static_cast<unsigned int>(0), sp_list.size()) << "List not empty.";

  unsigned int e = l;
  while (e == l)
    e = base::random_32bit_uinteger() % 10;

  // Delete random share by msid
  ASSERT_EQ(0, psh_->MI_DeletePrivateShare("MSID_" + base::itos(e), 1)) <<
            "Failed to delete.";

  // Full share list
  ASSERT_EQ(0, psh_->MI_GetFullShareList(&full_share_list)) <<
            "Failed getting full list";
  ASSERT_EQ(static_cast<unsigned int>(8), full_share_list.size()) <<
            "Share container not empty on creation.";

  // Find by share name
  ASSERT_EQ(-2014, psh_->MI_GetShareInfo("NAME_" + base::itos(e), 0, &by_name))
            << "Located share by name.";
  // Find by share msid
  ASSERT_EQ(-2014, psh_->MI_GetShareInfo("MSID_" + base::itos(e), 1, &by_msid))
            << "Located share by msid.";
  // Find the participants of the share
  ASSERT_EQ(-2015, psh_->MI_GetParticipantsList("MSID_" + base::itos(e), 1,
            &sp_list)) << "Some participants of the share remain.";
}

TEST_F(PrivateSharesTest, BEH_MAID_MI_AddContactToShare) {
  // Test share list to be empty
  std::list<maidsafe::private_share> share_list;
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(0), share_list.size()) <<
            "Share container not empty on creation.";

  // Copy the list for comparison
  std::list<maidsafe::ShareParticipants> cp = participants;

  // Add private share
  ASSERT_EQ(0, psh_->MI_AddPrivateShare(attributes, &cp)) <<
            "Failed to add share";

  // Check with GetShareInfo by name
  maidsafe::PrivateShare by_name;
  ASSERT_EQ(0, psh_->MI_GetShareInfo(attributes[0], 0, &by_name)) <<
            "Failed to locate share by name";

  // Check Participants with share msid
  std::list<maidsafe::share_participant> sp_list;
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[1], 1, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(2), participants.size()) <<
            "Probably forgot to refill original list.";
  ASSERT_EQ(participants.size(), sp_list.size()) <<
            "Retrived list size does not match.";

  // Add contact by msid
  std::list<maidsafe::ShareParticipants> sps;
  for (int a = 0; a < 3; a++) {
    maidsafe::ShareParticipants sp;
    sp.id = "PUB_NAME_" + base::itos(a);
    sp.public_key = "PUB_NAME_PUB_KEY_" + base::itos(a);
    sp.role = 'N';
    sps.push_back(sp);
  }
  ASSERT_EQ(0, psh_->MI_AddContactsToPrivateShare(attributes[1], 1,
            &sps)) << "Failed to add contacts.";

  // Get list by share name
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[0], 0, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(5), sp_list.size()) <<
            "Retrived list size does not match.";

  // Add same contacts by msid
  sps.clear();
  for (int a = 0; a < 3; a++) {
    maidsafe::ShareParticipants sp;
    sp.id = "PUB_NAME_" + base::itos(a);
    sp.public_key = "PUB_NAME_PUB_KEY_" + base::itos(a);
    sp.role = 'N';
    sps.push_back(sp);
  }
  ASSERT_EQ(0, psh_->MI_AddContactsToPrivateShare(attributes[1], 1,
            &sps)) << "Failed to add contacts.";

  // Get list by share msid
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[1], 1, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(5), sp_list.size()) <<
            "Retrived list size does not match.";

  // Add more contacts by name
  sps.clear();
  for (int a = 3; a < 7; a++) {
    maidsafe::ShareParticipants sp;
    sp.id = "PUB_NAME_" + base::itos(a);
    sp.public_key = "PUB_NAME_PUB_KEY_" + base::itos(a);
    sp.role = 'N';
    sps.push_back(sp);
  }
  ASSERT_EQ(0, psh_->MI_AddContactsToPrivateShare(attributes[0], 0,
            &sps)) << "Failed to add contacts.";

  // Get list by share name
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[0], 0, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(9), sp_list.size()) <<
            "Retrived list size does not match.";

  int n = 0;
  while (!sp_list.empty()) {
    ASSERT_EQ(attributes[1], sp_list.front().msid_) << "Msid wrong.";
    if (sp_list.front().public_name_ != "Dan" &&
        sp_list.front().public_name_ != "The Hutch") {
      ASSERT_EQ("PUB_NAME_" + base::itos(n), sp_list.front().public_name_) <<
                "Pub name wrong.";
      ASSERT_EQ("PUB_NAME_PUB_KEY_" + base::itos(n),
                sp_list.front().public_key_) << "Msid wrong.";
      ASSERT_EQ('N', sp_list.front().role_) << "Msid wrong.";
      n++;
    }
    sp_list.pop_front();
  }
}

TEST_F(PrivateSharesTest, BEH_MAID_MI_DeleteContactFromShare) {
  // Test share list to be empty
  std::list<maidsafe::private_share> share_list;
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(0), share_list.size()) <<
            "Share container not empty on creation.";

  // Copy the list for comparison
  std::list<maidsafe::ShareParticipants> cp = participants;

  // Add private share
  ASSERT_EQ(0, psh_->MI_AddPrivateShare(attributes, &cp)) <<
            "Failed to add share";

  // Check with GetShareInfo by name
  maidsafe::PrivateShare by_name;
  ASSERT_EQ(0, psh_->MI_GetShareInfo(attributes[0], 0, &by_name)) <<
            "Failed to locate share by name";

  // Add contact by msid
  std::list<maidsafe::ShareParticipants> sps;
  for (int a = 0; a < 7; a++) {
    maidsafe::ShareParticipants sp;
    sp.id = "PUB_NAME_" + base::itos(a);
    sp.public_key = "PUB_NAME_PUB_KEY_" + base::itos(a);
    sp.role = 'N';
    sps.push_back(sp);
  }
  ASSERT_EQ(0, psh_->MI_AddContactsToPrivateShare(attributes[1], 1,
            &sps)) << "Failed to add contacts.";

  // Get list by share name
  std::list<maidsafe::share_participant> sp_list;
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[0], 0, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(9), sp_list.size()) <<
            "Retrived list size does not match.";

  // Delete random contact
  unsigned int l = base::random_32bit_uinteger() % 7;
  std::list<std::string> del_list;
  del_list.push_back("PUB_NAME_" + base::itos(l));
  ASSERT_EQ(0, psh_->MI_DeleteContactsFromPrivateShare(attributes[1], 1,
            &del_list)) << "Failed to delete the participant.";

  // Get list by share name
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[0], 0, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(8), sp_list.size()) <<
            "Retrived list size does not match.";

  // Try to delete same participant from same share
  del_list.push_back("PUB_NAME_" + base::itos(l));
  ASSERT_EQ(-2013, psh_->MI_DeleteContactsFromPrivateShare(attributes[1], 1,
            &del_list)) << "Failed to delete the participant.";
  ASSERT_EQ(static_cast<unsigned int>(8), sp_list.size()) <<
            "Retrived list size does not match.";

  // create new share details
  std::string msid1 = attributes[1];
  attributes.clear();
  attributes.push_back("Nalga share");
  attributes.push_back(base::RandomString(64));
  attributes.push_back(base::RandomString(512));
  attributes.push_back(base::RandomString(512));
  for (int a = 0; a < 7; a++) {
    maidsafe::ShareParticipants sp;
    sp.id = "PUB_NAME_" + base::itos(a);
    sp.public_key = "PUB_NAME_PUB_KEY_" + base::itos(a);
    sp.role = 'N';
    sps.push_back(sp);
  }

  // Add private share
  ASSERT_EQ(0, psh_->MI_AddPrivateShare(attributes, &sps)) <<
            "Failed to add share";
  ASSERT_EQ(0, psh_->MI_GetShareList(&share_list)) << "Failed getting list.";
  ASSERT_EQ(static_cast<unsigned int>(2), share_list.size()) <<
            "Share container empty after insertions.";

  // Get list by share name
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[0], 0, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(7), sp_list.size()) <<
            "Retrived list size does not match.";

  // New random participant to delete
  unsigned int e = l;
  while (e == l)
    e = base::random_32bit_uinteger() % 7;
  del_list.push_back("PUB_NAME_" + base::itos(e));
  ASSERT_EQ(0, psh_->MI_DeleteContactsFromPrivateShare(attributes[1], 1,
            &del_list)) << "Failed to delete the participant.";

  // Get list by share name
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(attributes[0], 0, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(6), sp_list.size()) <<
            "Retrived list size does not match.";

  // Check other share to see that the contact
  // wasn't deleted from the other share
  ASSERT_EQ(0, psh_->MI_GetParticipantsList(msid1, 1, &sp_list)) <<
            "Failed to acquire participant list.";
  ASSERT_EQ(static_cast<unsigned int>(8), sp_list.size()) <<
            "Retrived list size does not match.";
  bool found = false;
  while (!sp_list.empty() && !found) {
    if (sp_list.front().public_name_ == "PUB_NAME_" + base::itos(e))
      found = true;
    sp_list.pop_front();
  }
}
