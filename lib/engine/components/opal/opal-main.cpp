
/* Ekiga -- A VoIP and Video-Conferencing application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * Ekiga is licensed under the GPL license and as a special exception,
 * you have permission to link or otherwise combine this program with the
 * programs OPAL, OpenH323 and PWLIB, and distribute the combination,
 * without applying the requirements of the GNU GPL to the OPAL, OpenH323
 * and PWLIB programs, as long as you do follow the requirements of the
 * GNU GPL for all the rest of the software thus combined.
 */


/*
 *                         opal-main.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Damien Sandras
 *   copyright            : (c) 2007 by Damien Sandras
 *   description          : code to hook Opal into the main program
 *
 */

#include "config.h"

#include "opal-main.h"

#include "account-core.h"
#include "chat-core.h"
#include "presence-core.h"
#include "audioinput-core.h"
#include "audiooutput-core.h"
#include "videoinput-core.h"
#include "videooutput-core.h"

#include "opal-plugins-hook.h"

#include "sip-endpoint.h"

// opal manages its endpoints itself, so we must be wary
struct null_deleter
{
    void operator()(void const *) const
    { }
};

static bool
is_supported_address (const std::string uri)
{
#ifdef HAVE_H323
  if (uri.find ("h323:") == 0)
    return true;
#endif

  if (uri.find ("sip:") == 0)
    return true;

  return false;
}

/* FIXME: add here an Ekiga::Service which will add&remove publishers,
 * decorators and fetchers
 */

using namespace Opal;

struct OPALSpark: public Ekiga::Spark
{
  OPALSpark (): result(false)
  {}

  bool try_initialize_more (Ekiga::ServiceCore& core,
			    int* /*argc*/,
			    char** /*argv*/[])
  {
    boost::shared_ptr<Ekiga::ContactCore> contact_core = core.get<Ekiga::ContactCore> ("contact-core");
    boost::shared_ptr<Ekiga::PresenceCore> presence_core = core.get<Ekiga::PresenceCore> ("presence-core");
    boost::shared_ptr<Ekiga::CallCore> call_core = core.get<Ekiga::CallCore> ("call-core");
    boost::shared_ptr<Ekiga::ChatCore> chat_core = core.get<Ekiga::ChatCore> ("chat-core");
    boost::shared_ptr<Ekiga::AccountCore> account_core = core.get<Ekiga::AccountCore> ("account-core");
    boost::shared_ptr<Ekiga::AudioInputCore> audioinput_core = core.get<Ekiga::AudioInputCore> ("audioinput-core");
    boost::shared_ptr<Ekiga::VideoInputCore> videoinput_core = core.get<Ekiga::VideoInputCore> ("videoinput-core");
    boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput_core = core.get<Ekiga::AudioOutputCore> ("audiooutput-core");
    boost::shared_ptr<Ekiga::VideoOutputCore> videooutput_core = core.get<Ekiga::VideoOutputCore> ("videooutput-core");
    boost::shared_ptr<Ekiga::PersonalDetails> personal_details = core.get<Ekiga::PersonalDetails> ("personal-details");
    boost::shared_ptr<Bank> account_store = core.get<Bank> ("opal-account-store");
    Ekiga::ServicePtr sip_endpoint = core.get ("opal-sip-endpoint");

    if (contact_core && presence_core && call_core && chat_core
	&& account_core && audioinput_core && videoinput_core
	&& audiooutput_core && videooutput_core && personal_details
	&& !account_store && !sip_endpoint) {

      PIPSocket::SetSuppressCanonicalName (true);  // avoid long delays

      hook_ekiga_plugins_to_opal (core);

      boost::shared_ptr<CallManager> call_manager (new CallManager (core));
      core.add (call_manager);

      boost::shared_ptr<Sip::EndPoint> sip_manager (new Sip::EndPoint (*call_manager, core), null_deleter ());
      std::cout << "FIXME: where is H323" << std::endl << std::flush;
      sip_manager->setup ();
      call_manager->set_sip_endpoint (sip_manager);
      core.add (sip_manager);

      boost::shared_ptr<Bank> bank (new Bank (core));
      account_core->add_bank (bank);
      presence_core->add_cluster (bank);
      core.add (bank);
      contact_core->add_contact_decorator (bank);
      presence_core->add_presentity_decorator (bank);
      call_manager->ready.connect (boost::bind (&Opal::Bank::call_manager_ready, &*bank));
      call_manager->setup ();
      presence_core->add_presence_publisher (bank);
      presence_core->add_presence_fetcher (bank);

      call_core->add_manager (call_manager);

      presence_core->add_supported_uri (&is_supported_address); //FIXME

      result = true;
    }

    return result;
  }

  Ekiga::Spark::state get_state () const
  { return result?FULL:BLANK; }

  const std::string get_name () const
  { return "OPAL"; }

  bool result;
};

void
opal_init (Ekiga::KickStart& kickstart)
{
  boost::shared_ptr<Ekiga::Spark> spark(new OPALSpark);
  kickstart.add_spark (spark);
}

