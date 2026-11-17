# Ordinary fresh pre-numbered item tracking

Base: 2.2.4-smithing-test3. Metadata2.2.4.4. No changes to recipe storage, FindSmithingInstance/PrepareSmithingInstance, Equipment.cpp or ReclaimManagedItems; these are byte-compared to test3. Smithing.h and SmithingTest.cpp also remain identical.

User trace for GR Inari EarsTails SMP.esp local000835 (runtimeFE2CE835) shows owned0 -> gallery creates1 -> synchronous unique-ID/container events give actor14 UID1 -> existing-id-compatibility-off. This establishes the skipped tracking boundary without attributing why the engine preassigns IDs.

During only AddManagedItems(previouslyAbsent=true,count=1), an RAII-scoped OrdinaryAddition observes the synchronous container event. It accepts exactly from0/toactor, the matching form, count1, nonzero UID and still-pending singleton. Outgoing movement marks the observation cancelled. It does not call engine code under the event mutex or claim an item in the event sink. Only scalar identity evidence is retained until TrackNewManagedItems.

Track consumes the evidence independently of cleanupCompatibility, using the existing inventory query. Adoption still requires pending provenance, matching observed/current owner+UID, total1, singleton1, one extra list, unique actor-local UID and no original/managed/reserved UID conflict. Revert/load and normal begin/consume lifecycle clear transient observations. Ordinary manual transfers/pickups outside our AddObject call cannot create this evidence.

Existing compatibility post-generation observation remains optional and unchanged; its name handling and transfer-chain behavior are not globally enabled. Reserved follower-original and retained-reproduction IDs are included in ordinary allocation/adoption exclusions. No ordinary item's preassigned UID is rewritten. Unproven/ambiguous identities remain untracked, not deleted by base form.

No extra GetInventory, polling, hooks, settings, serialization records or widget changes. Performance still requires runtime confirmation for this revised path, though no extra inventory scans were introduced. Automated cases cover scoped incoming evidence, absent/inactive pending, outgoing cancellation, wrong form/owner/count/UID/source, ambiguous lists, duplicates and original reservation exclusion.

Runtime tail adoption/reclaim and all previously unconfirmed smithing scenarios are outstanding; see VALIDATION-STATUS.ja.txt. Earlier test archives and the user-confirmed UI checkpoint remain unchanged.
