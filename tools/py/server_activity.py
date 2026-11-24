"""Who is on a live MK3D server: connects (no login), says hello as an MK3D launcher would, asks for the online
counts and prints them, and with --product the games its welcome lists for that product.

  python -I server_activity.py <mk3d-rollback repo> <host> <port> <fingerprint> [product]
"""
import asyncio
import json
import sys

repo, host, port, fp = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
product = sys.argv[5] if len(sys.argv) > 5 else None
sys.path.insert(0, repo)
from server import protocol as P  # noqa: E402
from server.client import Connection, ServerError  # noqa: E402


async def main():
    conn = Connection(host, port, fingerprint=fp)
    await conn.connect(hello=False)
    hello = {"type": "hello", "proto": P.PROTO, "client": "activity-check", "version": "1.0.16", "features": []}
    if product:
        hello["product"] = product
        hello["version"] = "0.1.0"
    try:
        welcome = await conn.request(hello)
    except ServerError as e:
        print("hello refused:", e)
        return
    print("server", welcome.get("server"), welcome.get("version"), "games", sorted(welcome.get("games", {})))
    online = await conn.request({"type": "online"})
    keep = {k: online.get(k) for k in ("count", "connected", "queued", "playing")}
    print("online", json.dumps(keep), "lobbies", {g: v.get("lobbies") for g, v in online.get("games", {}).items()})
    await conn.close()


asyncio.run(main())
