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

    with urllib.request.urlopen(owned_games_url) as response:
        data = json.loads(response.read().decode())
        games = data['response'].get('games', [])

    game_names = [game['name'] for game in games] if games else []
    game_ids = [game[str('appid')] for game in games] if games else []
    return game_names, game_ids
