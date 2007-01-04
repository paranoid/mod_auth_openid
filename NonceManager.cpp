#include "moid.h"

namespace modauthopenid {

  using namespace std;

  NonceManager::NonceManager(const string& storage_location)  : db_(NULL, 0) {
    u_int32_t oFlags = DB_CREATE; // Open flags;
    try {
      db_.open(NULL,                // Transaction pointer
	       storage_location.c_str(),          // Database file name
	       "nonces",                // Optional logical database name
	       DB_BTREE,            // Database access method
	       oFlags,              // Open flags
	       0);                  // File mode (using defaults)
      db_.set_errpfx("mod_openid bdb: ");
      db_.set_error_stream(&cerr); //this is apache's log
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database open failed %s", storage_location.c_str());
    } catch(exception &e) {
      db_.errx("Error opening database: %s", e.what());
    }
  };
  
  bool NonceManager::is_valid(const string& nonce, bool delete_on_find) {
    ween_expired();
    Dbt data;
    NONCE n;
    char id[255];
    strcpy(id, nonce.c_str());
    Dbt key(id, strlen(id) + 1);

    data.set_data(&n);
    data.set_ulen(sizeof(NONCE));
    data.set_flags(DB_DBT_USERMEM);
    if(db_.get(NULL, &key, &data, 0) == DB_NOTFOUND) {
      fprintf(stderr, "Failed Auth Attempt: Could not find nonce \"%s\" in nonce db.\n", nonce.c_str());
      return false;
    }
    if(delete_on_find) 
      db_.del(NULL, &key, 0);
    return true;
  };
  

  void NonceManager::add(const string& nonce) {
    ween_expired();
    time_t rawtime;
    time (&rawtime);
    NONCE n;
    n.expires_on = rawtime + 3600; // allow nonce to exist for one hour
    
    char id[255];
    strcpy(id, nonce.c_str());
    Dbt key(id, strlen(id) + 1);
    Dbt data(&n, sizeof(NONCE));
    db_.put(NULL, &key, &data, 0);
  };

  void NonceManager::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0) {
	NONCE *n = (NONCE *) data.get_data();
        if(rawtime > n->expires_on) {
          //fprintf(stderr, "Expires_on %i is greater than current time %i", data_v->expires_on, rawtime); fflush(stderr);
          db_.del(NULL, &key, 0);
        }
      }
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error!");
    } catch(std::exception &e) {
      db_.errx("Error! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
  };


  void NonceManager::close() {
    try {
      db_.close(0);
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database close failed");
    } catch(std::exception &e) {
      db_.errx("Error closing database: %s", e.what());
    }
  };
  
}