import asyncio
import hrgw
import argparse
import aiosqlite

class Impl(hrgw.Collector, hrgw.SleeperMixin):

    NAME = "Sqlite Datastore"

    def __init__(self):
        self.data = []
        self.running = True
        self.lock = asyncio.Lock()

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--data-store-flush-time", type=float, default=5,
                         help="Seconds between flushes to storage.")
        arg.add_argument("--data-store-db", type=str,
                         default="/tmp/DELME.db",
                         help="sqllite DB to use.")

    async def start(self, args):
        self.db = await aiosqlite.connect(args.data_store_db)
        await self.db.execute("""CREATE TABLE IF NOT EXISTS samples (
jd REAL,
name TEXT,
data REAL);""")
        while self.running:
            await self.sleep(args.data_store_flush_time)
            await self.flush()

    async def push(self, p:hrgw.Data):
        async with self.lock:
            self.data.append(p)

    async def stop(self):
        await self.flush()
        self.running = False

    async def flush(self):
        async with self.lock:
            await self.db.executemany(
                "INSERT INTO samples VALUES (?, ?, ?)", self.data)
            await self.db.commit()
            self.data = []
