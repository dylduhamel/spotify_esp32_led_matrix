# ESP32 Spotify LED Matrix Display

## Overview
This program is designed to interface with the Spotify API to fetch song information and display it on a 64x64 pixel LED matrix using an ESP32-Trinity board. It's compatible with ESP32 microcontrollers and most LED matrix boards, offering a unique way to visualize your Spotify music experience.

## Project Showcase
 ![musicimage](https://github.com/dylduhamel/spotify_esp32_led_matrix/assets/70403658/affa1b77-2eab-411c-8dfa-11e0c8e764c9)
 <img src="https://github.com/dylduhamel/spotify_esp32_led_matrix/assets/70403658/affa1b77-2eab-411c-8dfa-11e0c8e764c9" alt="Project Image" width="50%" height="50%">

## Spotify Account Setup
To use this program, you need to set up a Spotify Developer account and create an application. Follow these steps:

1. Sign into the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard/).
2. Create a new application (name it as you prefer).
3. Note down the "client ID" and "client secret" from your application page.

## Getting Your Refresh Token
Spotify's Authentication flow requires a webserver to complete, but it's only needed once to obtain your refresh token. This refresh token can then be used in all future sketches for authentication.

## ESP32-Trinity & Microcontroller libraries
Thank you to [Brian Lough](https://github.com/witnessmenow) for all of the incredible work with ESP32 Libraries.
