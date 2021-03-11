import asyncio
import hrgw
import argparse
import re


class Impl(hrgw.Producer):

    NAME = "Pinger"
    MATCH_LOSS = re.compile(r"([\d.]+)% packet loss", re.MULTILINE)
    MATCH_LAT = re.compile(
        r"rtt min/avg/max/mdev = ([\d.]+)/([\d.]+)/([\d.]+)/([\d.]+) ms",
        re.MULTILINE)

    def __init__(self):
        self.running = True
        self.proc = None

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--pinger-c", type=int, default=10,
                         help="ping -c.")
        arg.add_argument("--pinger-w", type=int, default=11,
                         help="ping -w.")
        arg.add_argument("--pinger-ip", type=str, default="8.8.8.8",
                         help="ping destination IP.")

    async def start(self, args, coll: hrgw.Collector):
        ip = args.pinger_ip
        if ip == "":
            return
        while self.running:
            self.proc = await asyncio.create_subprocess_exec(
                "ping", "-n", "-q",
                "-c", f"{args.pinger_c}",
                "-w", f"{args.pinger_w}",
                ip,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE)
            if not self.running:
                break
            stdout, stderr = await self.proc.communicate()
            if self.proc.returncode not in (0, 1) and self.running:
                print("Pinger execution error: ", self.proc.returncode)
                continue
            self.proc = None
            if not self.running:
                break
            t = stdout.decode(encoding="utf8", errors="ignore")
            m = self.MATCH_LOSS.search(t)
            if m is not None:
                await coll.push(f"P%_{ip}", float(m.group(1)))
            m = self.MATCH_LAT.search(t)
            if m is not None:
                await coll.push(f"PL_{ip}", float(m.group(2)))
            else:
                await coll.push(f"PL_{ip}", 0)

    async def stop(self):
        self.running = False
        if self.proc:
            try:
                self.proc.terminate()
            except ProcessLookupError:
                pass
