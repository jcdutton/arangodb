////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_REST_HANDLER_REST_REPLICATION_HANDLER_H
#define ARANGOD_REST_HANDLER_REST_REPLICATION_HANDLER_H 1

#include "Basics/Common.h"

#include "RestHandler/RestVocbaseBaseHandler.h"
#include "VocBase/replication-common.h"

namespace arangodb {
class ClusterInfo;
class CollectionNameResolver;
class LogicalCollection;
class Result;

namespace transaction {
class Methods;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief replication request handler
////////////////////////////////////////////////////////////////////////////////

class RestReplicationHandler : public RestVocbaseBaseHandler {
  // Never instantiate this.
  // Only specific implementations allowed
 protected:
  RestReplicationHandler(GeneralRequest*, GeneralResponse*);
  ~RestReplicationHandler();

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief creates an error if called on a coordinator server
  //////////////////////////////////////////////////////////////////////////////

  bool isCoordinatorError();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief turn the server into a slave of another
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandMakeSlave();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief forward a command in the coordinator case
  //////////////////////////////////////////////////////////////////////////////

  void handleTrampolineCoordinator();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the cluster inventory, only on coordinator
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandClusterInventory();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreIndexes();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  void handleCommandRestoreData();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Grant temporary restore rights
  //////////////////////////////////////////////////////////////////////////////

  void grantTemporaryRights();

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of the _users collection
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreUsersBatch(std::string const& colName, bool useRevision);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of a collection
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreDataBatch(transaction::Methods& trx,
                                 std::string const& colName, bool useRevision);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the data of a collection
  //////////////////////////////////////////////////////////////////////////////

  Result processRestoreData(std::string const& colName, bool useRevision);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief parse an input batch
  //////////////////////////////////////////////////////////////////////////////

  Result parseBatch(std::string const& collectionName, bool useRevision,
                    std::unordered_map<std::string, VPackValueLength>& latest,
                    VPackBuilder& allMarkers);

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// SECTION:
  /// Functions to be implemented by specialisation
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the state of the replication logger
  /// @route GET logger-state
  /// @caller Syncer::getMasterState
  /// @response VPackObject describing the ServerState in a certain point
  ///           * state (server state)
  ///           * server (version / id)
  ///           * clients (list of followers)
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandLoggerState() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a follow command for the replication log
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandLoggerFollow() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle the command to determine the transactions that were open
  /// at a certain point in time
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandDetermineOpenTransactions() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a batch command
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandBatch() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the inventory (current replication and collection state)
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandInventory() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief handle a restore command for a specific collection
  //////////////////////////////////////////////////////////////////////////////

  virtual void handleCommandRestoreCollection() = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the indexes of a collection TODO MOVE
  //////////////////////////////////////////////////////////////////////////////

  virtual int processRestoreIndexes(VPackSlice const&, bool, std::string&) = 0;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief restores the indexes of a collection, coordinator case
  //////////////////////////////////////////////////////////////////////////////

  virtual int processRestoreIndexesCoordinator(VPackSlice const&, bool, std::string&) = 0;
};

}  // namespace arangodb
#endif
