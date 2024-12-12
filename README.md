# Overview
This repository contains the implementation of a two-part project implemented in C regarding Systems Programming. 
The project involves developing a distributed client-server game where players control balls that move within a field to interact with bots and collect prizes.

## Part A: ChaseThem
In this part, the game features a client-server architecture using Unix domain datagram sockets. Players control balls represented by letters in a terminal-based field. 
The server manages:
- Player connections and movements.
- Bots, which move randomly on the field.
- Prizes, which improve the health of players' balls when collected.
  
Key rules:
- Players gain or lose health depending on interactions with other balls (players), bots, or prizes.
- A ball’s health starts at 10 and decreases to 0 upon unfavourable interactions. If a ball’s health reaches 0, it is removed from the game.
- The game supports up to 10 players and bots simultaneously.

## Part B: BetterChaseThem
This second part builds upon part A, and introduces a more robust implementation with improvements.
This part utilises Internet domain stream sockets and incorporates multithreading.

Improvements:
- Health Recovery: When a ball’s health reaches 0, the player has 10 seconds to confirm if they want to continue playing. If confirmed, health resets to 10.
- Threaded Server Architecture: The server creates a dedicated thread for each client, improving scalability and responsiveness.
- Bots & Prizes Management: Bots move every 3 seconds, and new prizes appear every 5 seconds (managed entirely within the server).
- Synchronization: The server ensures proper synchronization to handle concurrent updates to the game state.
