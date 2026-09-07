# Persistence boundary — not implemented

After the gate, add a versioned USaveGame for company state, orders, game time, inventory provenance, IDs and deterministic RNG state. A successful write must be followed by a real reload check. Existing browser saves and their migrations are not replaced or declared UE-compatible by this preparation.
