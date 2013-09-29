/*
   Vimpc
   Copyright (C) 2010 - 2012 Nathan Sweetman

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   mpdclient.hpp - provides interaction with the music player daemon
   */

#ifndef __MPC__CLIENT
#define __MPC__CLIENT

#include <thread>
#include <functional>

#include <mpd/client.h>

#include "output.hpp"
#include "screen.hpp"
#include "buffers.hpp"
#include "buffer/library.hpp"
#include "buffer/list.hpp"
#include "window/debug.hpp"

// The library check in 2.1.0 doesn't seem to work
// since we don't support versions older than that anyway, just return false
// instead of using the check macro
#if ((LIBMPDCLIENT_MAJOR_VERSION <= 2) && (LIBMPDCLIENT_MINOR_VERSION <= 1))
#undef LIBMPDCLIENT_CHECK_VERSION
#define LIBMPDCLIENT_CHECK_VERSION(major, minor, patch) \
    ((major) < LIBMPDCLIENT_MAJOR_VERSION || \
     ((major) == LIBMPDCLIENT_MAJOR_VERSION && \
      ((minor) < LIBMPDCLIENT_MINOR_VERSION || \
       ((minor) == LIBMPDCLIENT_MINOR_VERSION && \
        (patch) <= LIBMPDCLIENT_PATCH_VERSION))))
#endif

#ifndef LIBMPDCLIENT_MAJOR_VERSION
#define LIBMPDCLIENT_MAJOR_VERSION 0
#endif
#ifndef LIBMPDCLIENT_MINOR_VERSION
#define LIBMPDCLIENT_MINOR_VERSION 0
#endif
#ifndef LIBMPDCLIENT_PATCH_VERSION
#define LIBMPDCLIENT_PATCH_VERSION 0
#endif

namespace Main
{
   class Settings;
   class Vimpc;
}

namespace Ui
{
   class Screen;
}

// \todo cache all the values that we can
namespace Mpc
{
   class Client;
   class Output;
   class Song;

   uint32_t SecondsToMinutes(uint32_t duration);
   uint32_t RemainingSeconds(uint32_t duration);

   class CommandList
   {
      public:
         CommandList(Mpc::Client & client, bool condition = true);
         ~CommandList();

      private:
         bool          condition_;
         Mpc::Client & client_;
   };

   class Client
   {
      friend class Mpc::CommandList;

   public:
      Client(Main::Vimpc * vimpc, Main::Settings & settings, Ui::Screen & screen);
      ~Client();

   public:
      void QueueCommand(std::function<void()> function);
      void WaitForCompletion();

   private:
      Client(Client & client);
      Client & operator=(Client & client);

   public:
      // Mpd Connections
      void Connect(std::string const & hostname = "", uint16_t port = 0, uint32_t timeout_ms = 0);
      void ConnectImpl(std::string const & hostname = "", uint16_t port = 0, uint32_t timeout_ms = 0);
      void Disconnect();
      void Reconnect();
      void Password(std::string const & password);

   public:
      // Playback functions
      void Play(uint32_t playId);
      void Pause();
      void Stop();
      void Next();
      void Previous();
      void Seek(int32_t Offset);
      void SeekTo(uint32_t Time);
      void SeekToPercent(double Percent);

   public:
      //! \TODO add toggle versions for use by the player
      //! \TODO add a delta based volume control
      //! so that state will be correct
   
      // Toggle settings
      void SetRandom(bool random);
      void SetSingle(bool single);
      void SetConsume(bool consume);
      void SetRepeat(bool repeat);
      void SetCrossfade(bool crossfade);
      void SetCrossfade(uint32_t crossfade);
      void SetVolume(uint32_t volume);
      void SetMute(bool mute);

   public:
      // Playlist editing
      void Shuffle();
      void Move(uint32_t position1, uint32_t position2);
      void Swap(uint32_t position1, uint32_t position2);

   public:
      // Playlist management
      void CreatePlaylist(std::string const & name);
      void SavePlaylist(std::string const & name);
      void LoadPlaylist(std::string const & name);
      void RemovePlaylist(std::string const & name);
      void AddToNamedPlaylist(std::string const & name, Mpc::Song * song);

   public:
      // Outputs
      void SetOutput(Mpc::Output * output, bool enable);
      void EnableOutput(Mpc::Output * output);
      void DisableOutput(Mpc::Output * output);

   public:
      // Queue manipulation
      void Add(Mpc::Song * song);
      void Add(Mpc::Song & song);
      void Add(Mpc::Song & song, uint32_t position);
      void AddAllSongs();

      //! This add is only used by a command when a full uri is specified
      //! it should not be used to add songs from the library, you should use the
      //! versions above for that purpose
      void Add(std::string const & URI);

      // Call after all songs have been added
      void AddComplete();

      void Delete(uint32_t position);
      void Delete(uint32_t position1, uint32_t position2);
      void Clear();

   public:
      // Searching the database
      void SearchAny(std::string const & search, bool exact = false);
      void SearchArtist(std::string const & search, bool exact = false);
      void SearchAlbum(std::string const & search, bool exact = false);
      void SearchGenre(std::string const & search, bool exact = false);
      void SearchSong(std::string const & search, bool exact = false);
      void AddAllSearchResults();
      void SearchResults(std::string const & name);

   public:
      // Mpd Status
      void DisplaySongInformation();

   public:
      // Database state
      void Rescan(std::string const & Path);
      void Update(std::string const & Path);
      void StartCommandList();
      void SendCommandList();
      void UpdateCurrentSong();
      void UpdateStatus(bool ExpectUpdate = false);

   public:
      template <typename Object>
      void ForEachPlaylistSong(std::string Playlist, Object & object, void (Object::*callBack)(Mpc::Song *));

      template <typename Object>
      void ForEachSearchResult(Object & object, void (Object::*callBack)(Mpc::Song *));
      void GetAllOutputs();
      void GetAllMetaInformation();
      void GetAllMetaFromRoot();

   private:
      bool Connected() const;
      void IncrementTime(long time);
      void StateEvent();
      void CheckForEvents();
      void IdleMode();
      void ExitIdleMode();
      void ClientQueueExecutor(Mpc::Client * client);
      void SetStateAndEvent(int, bool & state, bool value);

   private:
      void ClearCommand();

   private:
      unsigned int QueueVersion();
      Song * CreateSong(uint32_t id, mpd_song const * const, bool songInLibrary = true) const;

   private:
      void GetVersion();
      bool CheckError();
      void DeleteConnection();

   private:
      Main::Vimpc *           vimpc_;
      Main::Settings &        settings_;
      struct mpd_connection * connection_;
      int                     fd_;

		std::thread	            clientThread_;

      std::string             hostname_;
      uint16_t                port_;
      uint32_t                versionMajor_;
      uint32_t                versionMinor_;
      uint32_t                versionPatch_;
      long                    timeSinceUpdate_;
      bool                    retried_;
      bool                    ready_;

      uint32_t                volume_;
      uint32_t                mVolume_;
      bool                    mute_;
      bool                    updating_;
      bool                    random_;
      bool                    repeat_;
      bool                    single_;
      bool                    consume_;
      bool                    crossfade_;
      uint32_t                crossfadeTime_;
      uint32_t                elapsed_;
      uint32_t                mpdelapsed_;
      mpd_state               state_;
      mpd_state               mpdstate_;

      struct mpd_song *       currentSong_;
      struct mpd_status *     currentStatus_;
      int32_t                 currentSongId_;
      uint32_t                totalNumberOfSongs_;
      std::string             currentSongURI_;
      std::string             currentState_;

      Ui::Screen &            screen_;
      int                     queueVersion_;
      bool                    forceUpdate_;
      bool                    listMode_;
      bool                    idleMode_;
   };

   //
   template <typename Object>
   void Client::ForEachPlaylistSong(std::string playlist, Object & object, void (Object::*callBack)(Mpc::Song * ))
   {
      QueueCommand([this, playlist, &object, callBack] ()
      {
         ClearCommand();

         if (Connected() == true)
         {
            Debug("Client::List songs in playlist %s", playlist.c_str());
            mpd_send_list_playlist(connection_, playlist.c_str());

            mpd_song * nextSong = mpd_recv_song(connection_);

            for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
            {
               Song * const song = Main::Library().Song(mpd_song_get_uri(nextSong));

               if (song != NULL)
               {
                  (object.*callBack)(song);
               }

               mpd_song_free(nextSong);
            }
         }
      });
   }

   // Requires search to be prepared before calling
   template <typename Object>
   void Client::ForEachSearchResult(Object & object, void (Object::*callBack)(Mpc::Song * ))
   {
      QueueCommand([this, &object, callBack] ()
      {
         if (Connected())
         {
            // Start the search
            Debug("Client::Commit search");
            mpd_search_commit(connection_);

            // Recv the songs and do some callbacks
            mpd_song * nextSong = mpd_recv_song(connection_);

            for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
            {
               Song * const song = Main::Library().Song(mpd_song_get_uri(nextSong));

               if (song != NULL)
               {
                  (object.*callBack)(song);
               }

               mpd_song_free(nextSong);
            }
         }
      });
   }

}

#endif
/* vim: set sw=3 ts=3: */
