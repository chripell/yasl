import hrgw
import re
import os
import argparse
import datetime
import time
import julian  # type: ignore


class Impl(hrgw.Producer, hrgw.SleeperMixin):

    NAME = "File Watch"

    def __init__(self):
        self.running = True

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--file-watchers", type=str, default="",
                         help="List of [file]:[seconds stalness].")

    async def start(self, args, coll: hrgw.Collector):
        cleaned: dict[str, str] = {}
        stalness: dict[str, float] = {}
        is_stale: dict[str, bool] = {}
        fws = [i.strip() for i in args.file_watchers.split(",")]
        now = julian.to_jd(datetime.datetime.now())
        for fw in fws:
            fname, stall = fw.split(":")
            cleaned[fname] = re.sub(r"\W", "_", fname)
            stalness[fname] = int(stall)
            is_stale[fname] = False
            await coll.push(f"FW_{cleaned[fname]}", 0, now)
        while self.running:
            await self.sleep(1.0)
            now = julian.to_jd(datetime.datetime.now())
            for fname in cleaned:
                mtime = os.stat(fname).st_mtime
                if time.time() - mtime > stalness[fname]:
                    if not is_stale[fname]:
                        await coll.push(f"FW_{cleaned[fname]}", 1, now)
                        is_stale[fname] = True
                else:
                    if is_stale[fname]:
                        await coll.push(f"FW_{cleaned[fname]}", 0, now)
                        is_stale[fname] = False

    async def stop(self):
        self.running = False
