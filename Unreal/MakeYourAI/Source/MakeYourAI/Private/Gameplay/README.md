# Gameplay boundary — not implemented

After the reference gate: location ownership, orders, inventory, mounting and upgrades belong in C++ domain operations, not a Level Blueprint. Match src/systems/procurement.ts and its tests. Available/Locked presentation must not invent a sequential unlock rule: the current base-location purchase function checks ownership, funds and board restrictions.
