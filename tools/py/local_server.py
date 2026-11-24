"""The MK3D server (with the arcade products) on 127.0.0.1 for local tests: a fresh database and certificate in the
folder given, fixed ports. Prints READY <tcp port> <fingerprint> once it listens, then runs until killed.

  python -I local_server.py <mk3d-rollback repo> <work folder> <tcp port> <udp port>
"""
import asyncio
import os
import sys

repo, work, tcp, udp = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
sys.path.insert(0, repo)
from server.app import Server, ServerConfig  # noqa: E402
from server.makecert import generate  # noqa: E402
from server.matchmaking import MatchmakingConfig  # noqa: E402


async def main():
    os.makedirs(work, exist_ok=True)
    fp = generate(os.path.join(work, "c.pem"), os.path.join(work, "k.pem"))
    cfg = ServerConfig(host="127.0.0.1", port=tcp, udp_port=udp, db_path=os.path.join(work, "s.sqlite"),
                       certfile=os.path.join(work, "c.pem"), keyfile=os.path.join(work, "k.pem"),
                       mm=MatchmakingConfig(allow_same_ip=True), relay_only=False)
    srv = Server(cfg)
    await srv.start()
    print("READY", srv.tcp_port, fp, flush=True)
    await asyncio.Event().wait()


asyncio.run(main())
