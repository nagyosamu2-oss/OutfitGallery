#include "GeneratedIdentity.h"
#include "DiagnosticSettings.h"
#include "ManagedLedger.h"
#include <iostream>
#include <stdexcept>
void Require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(){
 using namespace Gallery;
 Require(!cleanupCompatibility.load() && !DetailEnabled(),"support options default OFF");
 FreshAdditionObservation incoming{0x14,0x123};
 Require(incoming.Observe(0x123,0,0x14,1,1,true)==1,"capture engine preassigned UID during our singleton addition");
 Require(incoming.Observe(0x456,0,0x14,1,2,true)==0,"other item event ignored");
 Require(incoming.Observe(0x123,0,0x99,1,1,true)==0,"wrong destination ignored");
 Require(incoming.Observe(0x123,0,0x14,2,1,true)==0,"multiple items excluded");
 Require(incoming.Observe(0x123,0,0x14,1,0,true)==0,"no UID is not evidence");
 Require(incoming.Observe(0x123,0,0x14,1,1,false)==0,"no new pending claim excluded");
 Require(incoming.Observe(0x123,0x99,0x14,1,1,true)==0,"ordinary container transfer not creation");
 incoming.Observe(0x123,0x14,0x99,1,1,true);
 Require(incoming.cancelled && incoming.Observe(0x123,0,0x14,1,1,true)==0,"outgoing transfer permanently cancels this creation evidence");
 FreshAdditionObservation inactive;
 Require(inactive.Observe(0x123,0,0x14,1,1,true)==0,"inactive call cannot claim a pickup");
 const GeneratedIdentityEvidence good{true,true,0x14,0x14,0x14,42,42,1,1,1,1,false};
 Require(CanAdoptGeneratedIdentity(good),"fresh singleton is adoptable with compatibility and detail defaults OFF");
 Require(!CanAdoptGeneratedIdentity(good,2),"ambiguous form extra lists excluded");
 auto e=good;e.newPending=false;Require(!CanAdoptGeneratedIdentity(e),"preowned or cancelled claim");
 e=good;e.observedAfterCreation=false;Require(!CanAdoptGeneratedIdentity(e),"no generation evidence");
 e=good;e.actor=0;Require(!CanAdoptGeneratedIdentity(e),"null actor");
 e=good;e.observedOwner=0x99;Require(!CanAdoptGeneratedIdentity(e),"foreign source owner");
 e=good;e.currentOwner=0x99;Require(!CanAdoptGeneratedIdentity(e),"owner changed");
 e=good;e.observedID=0;e.currentID=0;Require(!CanAdoptGeneratedIdentity(e),"zero UID");
 e=good;e.currentID=43;Require(!CanAdoptGeneratedIdentity(e),"changed ID after equip");
 e=good;e.expectedCount=2;Require(!CanAdoptGeneratedIdentity(e),"multi item creation excluded");
 e=good;e.currentCount=2;Require(!CanAdoptGeneratedIdentity(e),"extra copy arrived");
 e=good;e.currentCount=0;Require(!CanAdoptGeneratedIdentity(e),"item vanished");
 e=good;e.instanceCount=2;Require(!CanAdoptGeneratedIdentity(e),"merged stack");
 e=good;e.matchingIDs=2;Require(!CanAdoptGeneratedIdentity(e),"duplicate UID anywhere in inventory");
 e=good;e.reserved=true;Require(!CanAdoptGeneratedIdentity(e),"saved enchantment or ledger reservation");
 e=good;e.actor=e.observedOwner=e.currentOwner=0xABCD;Require(CanAdoptGeneratedIdentity(e),"follower isolation");
 ManagedLedger ledger;
 ledger.pending[{0x14,0x123}]=1;
 ledger.Transfer(0x123,0x14,0);ledger.Transfer(0x123,0,0x14);
 auto pending=ledger.TakePending(0x14);e=good;e.newPending=pending.contains(0x123);
 Require(!CanAdoptGeneratedIdentity(e),"outgoing-and-returned item must not regain pending claim");
 ledger.items.insert({0x123,0x14,42});
 Require(ledger.Transfer(0x123,0x14,0)==1,"adopted ID relinquished on outgoing transfer");
 ledger.Transfer(0x123,0,0x14);Require(ledger.items.empty(),"returned item remains protected");
 std::cout<<"Generated identity safety scenarios passed\n";
}
