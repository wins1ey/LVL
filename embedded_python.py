def main():
    import urllib.request
    import json
    import os
    from dotenv import load_dotenv, find_dotenv

    try:
        load_dotenv(find_dotenv())
        api_key = os.environ.get('STEAM_API_KEY')
        steam_id = os.environ.get('STEAM_ID')
    except Exception as e:
        print(f"Failed to load environment variables: {e}")
        exit(1)

    base_url = 'http://api.steampowered.com/'
    player_summary_url = f'{base_url}ISteamUser/GetPlayerSummaries/v0002/?key={api_key}&steamids={steam_id}'
    owned_games_url = f'{base_url}IPlayerService/GetOwnedGames/v0001/?key={api_key}&steamid={steam_id}&format=json&include_appinfo=true'

    def get_player_summary():
        with urllib.request.urlopen(player_summary_url) as response:
            data = json.loads(response.read().decode())
            players = data['response']['players']
            if players:
                return players[0]
        return None

    def get_owned_games():
        with urllib.request.urlopen(owned_games_url) as response:
            data = json.loads(response.read().decode())
            games = data['response'].get('games', [])
            return games

    # Fetch player information
    player_info = get_player_summary()
    if player_info:
        print(f"Player Name: {player_info.get('personaname', 'Unknown')}")
    else:
        print("Failed to fetch player information.")

    # Fetch games owned by the user
    games = get_owned_games()
    if games:
        print(f"{player_info.get('personaname', 'The user')} owns {len(games)} games.")
        for game in games:
            print(f"{game['name']} - Playtime: {game['playtime_forever']} minutes")
    else:
        print("No games found or failed to fetch games.")

main()
