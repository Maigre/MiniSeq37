#include "Pattern.h"
#include<iostream>
using namespace std;

Pattern::Pattern() {
  pattclock = new Clock();
  notesON.resize(pattclock->ticksloop());
  program = NULL;
  volume = 100;
};

void Pattern::clear() {
  lock();
  for (uint k=0; k<notesON.size(); k++) notesON[k].clear();
  unlock();
  pattclock->clear();
}

bool Pattern::notempty() {
  for (uint k=0; k<notesON.size(); k++)
    if (!notesON[k].empty()) return true;
  return false;
};

Clock* Pattern::clock() {
  return pattclock;
}

void Pattern::resize() {
   resize(pattclock->ticksloop());
}

void Pattern::resize(uint64_t t) {
  if (t >= notesON.size())
    notesON.resize(t+2);
}

MMidiNote* Pattern::addNote(uint tick, uint note, uint duration) {
  return addNote(tick, note, duration, 64);
}

MMidiNote* Pattern::addNote(uint tick, uint note, uint duration, uint velocity) {
  resize(tick);
  MMidiNote* noteOn = new MMidiNote(note, velocity, duration);
  lock();
  notesON[tick].push_back(noteOn);
  unlock();
  return noteOn;
}

void Pattern::copyNotes(uint startCopy, uint startPast, uint count) {
  for (uint tick=0; tick<count; tick++) {
    if (startCopy+tick >= notesON.size()) break;
    if (startPast+tick >= notesON.size()) break;
    std::vector<MMidiNote*> buffer;
    buffer = getNotes(startCopy+tick, 1);
    lock();
    for ( int i = 0; i < notesON[startPast+tick].size(); i++ )
      delete notesON[startPast+tick][i];
    notesON[startPast+tick].clear();
    unlock();
    for (uint k=0; k<buffer.size(); k++)
      addNote(startPast+tick, buffer[k]->note, buffer[k]->length);

  }
}

std::vector<MMidiNote*> Pattern::getNotes(uint start, uint size) {
  return getNotes(start, size, -1);
}

std::vector<MMidiNote*> Pattern::getNotes(uint start, uint size, int noteval) {
  std::vector<MMidiNote*> notes;

  uint endT = start + size-1;
  if (endT >= notesON.size())
    endT = notesON.size()-1;

  lock();

  // Stack requested notes
  for (uint t=start; t<=endT; t++)
    for (uint k=0; k<notesON[t].size(); k++)
      if (noteval < 0 || (uint)noteval == notesON[t][k]->note)
        if (notesON[t][k]->isValid())
          notes.push_back(notesON[t][k]);

  unlock();
  return notes;
}

// Clean dead notes
void Pattern::cleanNotes(uint t) {
  if (t >= notesON.size()) return;
  bool clean = false;
  bool scanfrom = 0;
  lock();
  while (!clean) {
    clean = true;
    for (uint k=scanfrom; k<notesON[t].size(); k++)
      if (!notesON[t][k]->isValid() && !notesON[t][k]->isPlaying()) {
        notesON[t].erase(notesON[t].begin() + k);
        clean = false;
        scanfrom = k;
        break;
      }
  }
  unlock();
}

// get MMidiProgram
MMidiProgram* Pattern::getProgram() {
  return program;
}

// set bank+program
void Pattern::setProgram(uint _bank, uint _program) {
  lock();
  program = new MMidiProgram(_bank, _program);
  unlock();
}

// clear bank+program
void Pattern::clearProgram() {
  lock();
  program = NULL;
  unlock();
}

// Volume
void Pattern::setVolume(uint vol) {
  lock();
  volume = vol;
  unlock();
}

uint Pattern::getVolume() {
  return volume;
}

// Play program change
void Pattern::playProgram(ofxMidiOut* _out, u_int _chan) {
  if (program != NULL) program->play( _out, _chan);
};

// export memory
Json::Value Pattern::memdump() {

  if (!notempty()) return Json::nullValue;

  // Notes
  uint index = 0;
  memory["notes"].clear();
  for (uint t=0; t<notesON.size(); t++)
    for (uint k=0; k<notesON[t].size(); k++)
      if (notesON[t][k]->isValid()) {
        memory["notes"][index] = notesON[t][k]->memdump();
        memory["notes"][index]["tick"] = t;
        index++;
      }

  // Clock
  memory["clock"] = pattclock->memdump();

  // Volume
  memory["volume"] = volume;

  // Program
  if (program != NULL)
    memory["program"] = program->memdump();

  return memory;
}

// import memory
void Pattern::memload(Json::Value data) {
  MemObject::memload(data);

  // Clock
  pattclock->memload(data["clock"]);

  // Volume
  if (data.isMember("volume"))
    volume = data["volume"].asUInt();

  // Notes
  for (uint k=0; k<data["notes"].size(); k++)
    addNote(data["notes"][k]["tick"].asUInt(),
            data["notes"][k]["note"].asUInt(),
            data["notes"][k]["length"].asUInt(),
            data["notes"][k]["velocity"].asUInt());

  // Program
  if (data.isMember("program"))
    setProgram(data["program"]["bank"].asUInt(), data["program"]["prog"].asUInt());
}
