# NECRO MMO

> A C++ powered suite for building a **MMORPG**, inspired by the technical challenges of *World of Warcraft*.

<p align="center">
  <img width="100%" alt="NECRO MMO screenshot" src="https://github.com/silvematt/NECROEngine/assets/20478938/fa632904-6105-4b7a-8d44-ad700113e6fb">
</p>

---

## Main Features

- **Complete MMO suite**: Auth server, C++ client engine, game server (in progress), and a stress-testing tool.
- **Battle-tested paradigms**: WoW-style networking and gameplay concepts.
- **High-performance C++**: Core services implemented with `Boost.Asio` for scalable I/O.
- **Modular design**: Each component can evolve independently while sharing common protocols and data models.

---

## Architecture & Components

| Component       | Role                      | Tech               | Status                                                                 | Description |
|-----------------|---------------------------|--------------------|------------------------------------------------------------------------|-------------|
| [**NECROAuth**](/src/NECROAuth)    | Authentication Server     | C++ / `Boost.Asio` | ![functional](https://img.shields.io/badge/status-functional-brightgreen) | Account management with a MySQL database, login handshake, session tokens, and gateway to game realms. |
| [**NECROClient**](/src/NECROClient) | Game Engine & Client      | C++                | ![functional](https://img.shields.io/badge/status-functional-brightgreen) | Data-driven game engine, renders the world, handles player input, networking, entity sync, and client-side prediction. |
| [**NECROWorld**](/src/NECROWorld) | World Game Server               | C++ / `Boost.Asio` | ![not started](https://img.shields.io/badge/status-not_started-lightgrey) | Authoritative world state, zone/instance management, AI, scripts, combat, persistence with a MySQL database. |
| [**NECROHammer**](/src/NECROHammer) | Load & Stress Tester      | C++ / `Boost.Asio` | ![functional](https://img.shields.io/badge/status-functional-brightgreen) | Hammers servers with synthetic clients and scripted traffic to validate throughput and latency under high load. |

---
