import dothat.backlight as backlight
import dothat.lcd as lcd
import hrgw
import argparse
import typing
import time


class Impl(hrgw.Collector, hrgw.SleeperMixin):

    def __init__(self):
        self.monitored: typing.Dict[str, float] = {}
        self.name: typing.Dict[str, str] = {}
        self.running = True

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--disphat-values", type=str, default="",
                         help="Comma separated vars:name to display.")
        arg.add_argument("--disphat-alerts", type=str, default="",
                         help="Comma separated alert expressions.")
        arg.add_argument("--disphat-showtime", type=float, default=5.0,
                         help="Time to display each value.")
        arg.add_argument("--disphat-contrast", type=int, default=51,
                         help="Contrast of the LCD.")
        arg.add_argument("--disphat-hue", type=int, default=79,
                         help="Contrast of the LCD.")

    async def start(self, args):
        if args.disphat_values == "":
            ra = ["Void:V", "Void1:V1", "Void2:V2"]
        else:
            ra = [i.strip() for i in args.disphat_values.split(",")]
        n = len(ra)
        self.name = {k: v for (k, v) in (i.split(":") for i in ra)}
        a = [v for (v, _) in (i.split(":") for i in ra)]
        self.monitored = {k: 0.0 for k in a}
        if args.disphat_alerts == "":
            ral = []
        else:
            ral = [i.strip() for i in args.disphat_alerts.split(",")]
        alert_vars = {'V': self.monitored}
        alerts = [lambda: eval(i, alert_vars) for i in ral]
        cur = 0
        lcd.set_contrast(args.disphat_contrast)
        backlight.sweep(args.disphat_hue / 360.0, 0)
        refresh = time.time() + args.disphat_showtime
        alert_led = 0
        while self.running:
            await self.sleep(0.1)
            if any(a() for a in alerts):
                backlight.set_graph(alert_led / 20.0)
            else:
                backlight.set_graph(0)
            alert_led += 1
            alert_led = alert_led % 25
            if time.time() < refresh:
                continue
            refresh = time.time() + args.disphat_showtime
            nxt = (cur + 1) % n
            lcd.clear()
            self.show(0, a[cur])
            self.show(1, a[nxt])
            self.show(2, a[(nxt + 1) % n])
            if len(a) > 3:
                cur = nxt

    def show(self, row: int, k: str):
        lcd.set_cursor_position(0, row)
        n = self.name[k]
        d = self.monitored[k]
        lcd.write(f"{n} {d}")

    async def push(self, p: hrgw.Data):
        self.monitored[p.name] = p.data

    async def stop(self):
        self.running = False