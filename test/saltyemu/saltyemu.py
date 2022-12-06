import random

from fastapi import FastAPI, Form
from fastapi.responses import HTMLResponse
from pydantic import BaseModel


class Gamemode:
    matchmaking = 0
    tournament = 1
    exhibition = 2
    count = 3


class Status:
    open = 0
    close = 1
    winner = 2


def gamemode_count(gamemode: int) -> int:
    return [99, 16, 50][gamemode]


RANKS = list(enumerate([0, 25, 50, 75, 100,
                        150, 275, 400, 550, 700,
                        850, 1000, 1500, 2000, 2500,
                        4000, 5500, 7000, 9001, 15000,
                        25000, 35000, 50000]))[::-1]


class Player:
    def __init__(self, user_id: str, username: str, balance: int) -> None:
        self.user_id = user_id
        self.username = username
        self.balance = balance
        self.wager = 0
        self.player = 0
        self.tournament_end = balance
        self.total_bets = 0


def get_rank(total_bets: int) -> int:
    for rank, bets in RANKS:
        if total_bets >= bets:
            return rank
    raise Exception("total bets < 0")


def get_bailout(rank: int) -> int:
    return 1000 + 25 * rank


class SaltyMatch:
    def __init__(self, p1name: str, p2name: str, remaining: str,
                 p1total: int = 0, p2total: int = 0,
                 alert: str = "", x: int = 0, status: str = "open") -> None:
        self.p1name = p1name
        self.p2name = p2name
        self.remaining = remaining
        self.p1total = p1total
        self.p2total = p2total
        self.status = status
        self.alert = alert
        self.x = x
    
    def ser(self) -> dict[str, str | int]:
        return {
            "p1name": self.p1name,
            "p2name": self.p2name,
            "p1total": str(self.p1total),
            "p2total": str(self.p2total),
            "status": self.status,
            "alert": self.alert,
            "x": self.x,
            "remaining": self.remaining,
        }


class SaltyBetState:
    def __init__(self):
        self.player = Player("4204924", "testuser420", 420000)
        self.characters = ["Brandon", "Hayley", "Palace", "Parky", "Jacob", "Chelsea", "Yigs", "Spatnack", "Long name person mvc ssj+", "Team A", "Team B"]
        self.gamemode = Gamemode.matchmaking
        self.remaining = gamemode_count(self.gamemode)
        self.randomize_match()


    def get_remaining(self) -> tuple[str, str]:
        remaining = None
        alert = ""
        if self.remaining == 1:
            remaining = ["Tournament mode will be activated after the next match!",
                         "FINAL ROUND! Stay tuned for exhibitions after the tournament!",
                         "Matchmaking mode will be activated after the next exhibition match!"][self.gamemode]
        else:
            remaining = ["{} more matches until the next tournament!",
                         "{} characters are left in the bracket!",
                         "{} exhibition matches left!"][self.gamemode].format(self.remaining)
            if self.remaining == gamemode_count(self.gamemode):
                alert = "{} mode start!".format(["Matchmaking", "Tournament", "Exhibition"][self.gamemode])
        return (remaining, alert)


    def randomize_match(self):
        red, blue = random.sample(self.characters, 2)
        remaining, alert = self.get_remaining()
        self.current_match = SaltyMatch(red, blue, remaining, alert=alert)
    

    def next_match(self):
        if self.player.wager:
            self.player.total_bets += 1
        
        self.player.wager = 0
        self.player.player = 0

        self.remaining -= 1
        if self.remaining == 0:
            self.gamemode = (self.gamemode + 1) % Gamemode.count
            self.remaining = gamemode_count(self.gamemode)
            if self.gamemode == Gamemode.tournament:
                self.player.tournament_end = self.player.balance
                self.player.balance = get_bailout(get_rank(self.player.total_bets))
            elif self.gamemode == Gamemode.exhibition:
                self.player.balance += self.player.tournament_end

        self.randomize_match()
    

    def finish_match(self, winner: int):
        self.current_match.status = str(winner)
        if self.player.wager:
            if self.player.player == winner:
                p1t = self.current_match.p1total
                p2t = self.current_match.p2total
                p_t, o_t = (p1t, p2t) if self.player.player == 1 else (p2t, p1t)
                self.player.balance += round((self.player.wager / p_t) * o_t)
            else:
                self.player.balance = max(self.player.balance - self.player.wager, get_bailout(get_rank(self.player.total_bets)))


    def get_state(self) -> dict[str, str | int]:
        return self.current_match.ser()
    

    def get_player_state(self) -> dict[str, str]:
        return {
            "n": self.player.username,
            "b": str(self.player.balance),
            "p": str(self.player.player),
            "w": str(self.player.wager),
            "r": str(get_rank(self.player.total_bets)),  # rank
            "g":"0",    # gold
            "c":"0"     # color?
        }
    

    def place_wager(self, wager: int, player: int):
        self.player.wager = wager
        self.player.player = player
    

app = FastAPI()
salty = SaltyBetState()


class PlaceBet(BaseModel):
    selectedplayer: str
    wager: int


@app.get("/", response_class=HTMLResponse)
def root():
    return open("index.html", "r").read().format(name=salty.player.username,
                                                 user_id=salty.player.user_id,
                                                 balance=salty.player.balance)


@app.get("/state.json")
def get_state():
    return salty.get_state()


@app.get("/zdata.json")
def get_zdata():
    state: dict[str, str | int | dict[str, str]] = salty.get_state()  # type: ignore
    state[salty.player.user_id] = salty.get_player_state()
    if salty.current_match.status == "locked":
        salty.finish_match(1)
    elif salty.current_match.status in "12":
        salty.next_match()
    return state


@app.post("/ajax_place_bet.php")
def post_wager(selectedplayer: str = Form(), wager: int = Form()):
    player = 1 if "1" in selectedplayer else 2
    salty.place_wager(wager, player)
    if salty.gamemode == Gamemode.tournament:
        salty.current_match.p1total = random.randint(1000, 10000)
        salty.current_match.p2total = random.randint(1000, 10000)
    else:
        salty.current_match.p1total = random.randint(10000, 1000000)
        salty.current_match.p2total = random.randint(10000, 1000000)
    
    if player == 1:
        salty.current_match.p1total += wager
    else:
        salty.current_match.p2total += wager
    
    salty.current_match.status = "locked"


@app.get("/ajax_tournament_start.php", response_class=HTMLResponse)
def get_tournament_start():
    return str(get_bailout(get_rank(salty.player.total_bets)))


@app.get("/ajax_tournament_end.php", response_class=HTMLResponse)
def get_tournament_end():
    return str(salty.player.balance)
