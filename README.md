# fhq-jury-ad

[![Build Status](https://travis-ci.org/freehackquest/fhq-jury-ad.svg?branch=master)](https://travis-ci.org/freehackquest/fhq-jury-ad)

Jury System for a attack-defence ctf game (ctf-scoreboard).
Or you can use for training.

* [API: here](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/API.md)
* [DOCKER_HUB: description](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/DOCKER_HUB.md)
* [TRAINING MODE: lazy-start](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/LAZY_START.md)
* [SIMILAR: systems](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/SIMILAR.md)

## For Developers

* [BUILD: Ubuntu/Debian](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/BUILD_UBUNTU.md)
* [BUILD: MacOS](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/BUILD_MACOS.md)
* [BUILD: Docker](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/BUILD_DOCKER.md)

## Screens

![scoreboard](https://raw.githubusercontent.com/freehackquest/fhq-jury-ad/master/misc/screens/screen1.png)
![scoreboard-info](https://raw.githubusercontent.com/freehackquest/fhq-jury-ad/master/misc/screens/screen2.png)


## Example vulnerability services and checkers:

* [CHECKER_INFO: description](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/CHECKER_INFO.md)
* [example_service1](https://github.com/freehackquest/fhq-jury-ad/tree/master/vulnbox/example_service1)
* [Checker & Config for: example_service1](https://github.com/freehackquest/fhq-jury-ad/tree/master/jury.d/checkers/example_service1/)



### Download and basic configuration

```
$ sudo apt install git-core
$ cd ~
$ git clone http://github.com/freehackquest/fhq-jury-ad.git fhq-jury-ad.git
$ nano ~/fhq-jury-ad.git/jury.d/game.conf
$ nano ~/fhq-jury-ad.git/jury.d/server.conf
$ nano ~/fhq-jury-ad.git/jury.d/scoreboard.conf
$ nano ~/fhq-jury-ad.git/jury.d/mysql_storage.conf
```
Config files (look comments in file):
* `~/fhq-jury-ad.git/jury.d/game.conf` - basic game configurations, start/end/name/flag live time
* `~/fhq-jury-ad.git/jury.d/mysql_storage.conf` - basic db connection configurations
* `~/fhq-jury-ad.git/jury.d/scoreboard.conf` - basic scoreboard configurations html folder, port for web
* `~/fhq-jury-ad.git/jury.d/server.conf` - basic server just a which type of storage will be used (in current time worl only mysql)

* [BUILD: Ubuntu/Debian](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/BUILD_UBUNTU.md)
* [BUILD: MacOS](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/BUILD_MACOS.md)
* [BUILD: Docker](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/BUILD_DOCKER.md)

### Configure database

* [MYSQL DATABASE: create](https://github.com/freehackquest/fhq-jury-ad/blob/master/docs/STORAGE_MYSQL.md)

After configure database options here:

```
$ nano ~/fhq-jury-ad.git/jury.d/mysql_storage.conf
```

### Prepare to start with clean all previous data

Previously data-flags will be clean

```
$ cd ~/fhq-jury-ad.git/fhq-jury-ad
$ ./fhq-jury-ad clean
```

### Run jury-ad

```
$ ./jury-ad start
```

## Scoreboard

url: http://{HOST}:{PORT}/

Where

* {HOST} - host or ip, where jury system started
* {PORT} - scoreboard/flag port, where jury system started


### Service statuses

* up - put-check flag to service success
* down - service is not available (maybe blocked port or service is down)
* corrupt - service is available (available tcp connect) but protocol wrong could not put/get flag
* mumble - waited time (for example: 5 sec) but service did not have time to reply
* shit - checker not return correct response code

## Acceptance of flag

* Acceptance of flag: http://{HOST}:{PORT}/flag?teamid={TEAMID}&flag={FLAG}

Where

* {HOST} - host or ip, where jury system started
* {PORT} - scoreboard/flag port, where jury system started
* {TEAMID} - number, your unique teamid (see scoreboard)
* {FLAG} - uuid, so... it's flag from enemy server

Example of send flag (curl):

```
$ curl http://192.168.1.10:8080/flag?teamid=keva&flag=123e4567-e89b-12d3-a456-426655440000
```

http-code responses:

 * 400 - wrong parameters
 * 200 - flag accept
 * 403 - flag not accept (reason: old, already accepted, not found)

