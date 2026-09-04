/*
ESPTimeCast™

Copyright (c) 2026 M-Factory

This software is source-available for personal, non-commercial use only.
It is not open source.

See LICENSE.txt for full terms.

--------------------------------------
THIRD-PARTY COMPONENTS:
Lucide Icons
Licensed under the ISC License.
https://lucide.dev/license
--------------------------------------
*/

#pragma once
// index_html.h

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en" style="background: linear-gradient(135deg, rgb(8, 31, 86) 0%, rgb(17, 15, 46) 50%, rgb(68, 26, 101) 100%);
    height: 100%; background-attachment: fixed;">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <meta
      http-equiv="Cache-Control"
      content="no-cache, no-store, must-revalidate"
    />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta http-equiv="Expires" content="0" />
    <title>ESPTimeCast Settings</title>
    <style>
      *,
      *::before,
      *::after {
        box-sizing: border-box;
      }

      :root {
        --bg-gradient: linear-gradient(
          135deg,
          #081f56 0%,
          #110f2e 50%,
          #441a65 100%
        );
        --accent-color: #0ea5e9;
        --glass-bg: rgba(255, 255, 255, 0.05);
        --glass-border: rgba(255, 255, 255, 0.12);
      }

      .ssid-wrapper {
        position: relative;
      }

      .combo-container {
        display: flex;
        box-sizing: border-box;
        width: 100%;
        border: 1.5px solid rgba(180, 230, 255, 0.08);
        border-radius: 8px;
        background-color: rgba(225, 245, 255, 0.07);
        color: #ffffff;
        font-size: 1rem;
        appearance: none;
      }

      #ssid {
        border-radius: 8px 0 0 8px;
        flex: 1;
        border: none;
        outline: none;
        background: transparent;
      }

      .icon-btn {
        width: 40px;
        border: none;
        background: none;
        cursor: pointer;
        display: flex;
        align-items: center;
        justify-content: center;
        color: #666;
        transition: background 0.2s;
      }

      #arrowBtn {
        background: transparent;
        border: none;
        width: 40px;
        border-left: 1.5px solid rgba(180, 230, 255, 0.08);
      }

      #arrowBtn > svg {
        position: relative;
        top: 2px;
        filter: invert(0);
        opacity: 1;
      }

      #arrowBtn:disabled > svg {
        position: relative;
        top: 2px;
        opacity: 0.25;
      }

      #arrowBtn:hover {
        transform: translateY(-1px);
        box-shadow: 0 6px 16px rgba(0, 122, 255, 0.35);
      }

      #arrowBtn:disabled:hover {
        transform: translateY(0px);
        box-shadow: none;
      }

      #arrowBtn:disabled {
        cursor: not-allowed;
        background: none;
        color: rgba(255, 255, 255, 0.25);
      }

      #scanBtn {
        border-radius: 0 8px 8px 0;
        width: 75px;
        padding: 0;
        text-align: center;
        align-self: auto;
      }

      .icon-btn:hover {
        background: #f5f5f5;
      }

      #scanBtn:disabled {
        background: rgba(255, 255, 255, 0.5);
        cursor: wait;
      }

      #ssidList {
        position: absolute;
        width: 100%;
        max-height: 50vh;
        overflow-y: auto;
        background: white;
        border: 1px solid var(--border-color);
        border-radius: 6px;
        display: none;
        z-index: 1000;
        box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1);
      }

      .ssid-option {
        padding: 10px 12px;
        cursor: pointer;
        color: black;
      }

      .ssid-option:hover {
        background-color: var(--accent-color);
        color: white;
      }

      * {
        box-sizing: border-box;
      }

      html {
      }

      body {
        font-family: Roboto, system-ui;
        margin: 0;
        color: #ffffff;
        background-repeat: no-repeat, repeat, repeat;
        opacity: 0;
        transition: opacity 0.6s cubic-bezier(0.4, 0, 0.2, 1);
        visibility: hidden;
        line-height: 1.5;
        -webkit-font-smoothing: antialiased;
        -moz-osx-font-smoothing: grayscale;
        min-height: 100dvh;
        display: grid;
        place-items: center;
      }

      body.loaded {
        visibility: visible;
        opacity: 1;
      }

      body.modal-open {
        overflow: hidden;
      }

      h1 {
        text-align: center;
        font-size: 1.5rem;
        margin-bottom: 1.5rem;
        color: #ffffff;
      }

      h2 {
        font-size: 1rem;
        margin-bottom: 0.3rem;
        padding-left: 0.3rem;
      }

      h3 {
        font-weight: normal;
        font-size: 1.35rem;
        margin: 0.5rem 0 0 0;
      }

      #savingModalContent p {
        line-height: 1.6rem;
        font-size: 1rem;
      }

      .modal-buttons {
        display: flex;
        justify-content: center;
        gap: 1rem;
      }

      form:first-of-type > h2:first-of-type {
        margin-top: 1.5rem;
      }

      .logo {
        margin: auto;
        width: 90%;
        display: flex;
        flex-flow: column;
        align-items: center;
      }

      .logo svg {
        filter: drop-shadow(0px 5px 10px black);
        height: auto;
        margin: 0.5rem 0;
        width: 100%;
      }

      #device-status {
        min-height: 1.5rem;
        align-self: flex-end;
        padding-right: 0.25rem;
        opacity: 0;
        transition: opacity 0.6s cubic-bezier(0.4, 0, 0.2, 1);
        visibility: hidden;
      }

      #device-status.loaded {
        visibility: visible;
        opacity: 1;
      }

      form {
        /* background: var(--glass-bg);
        border: 1px solid var(--glass-border); */
        display: flex;
        flex-direction: column;
        max-width: 500px;
        margin: 0 auto;
        padding: 2.5rem 1.5rem 3.5rem 1.5rem;
        /* border-radius: 24px;
        box-shadow:
          0 10px 36px 0 rgba(40, 170, 255, 0.11),
          0 2px 8px 0 rgba(44, 70, 110, 0.08); */
        box-sizing: border-box;
      }

      label {
        font-size: 0.9rem;
        display: block;
        margin-top: 0.75rem;
      }

      input[type="text"],
      input[type="time"],
      input[type="password"],
      input[type="date"],
      input[type="number"],
      select {
        width: 100%;
        padding: 0.75rem;
        border: 1.5px solid rgba(180, 230, 255, 0.08);
        border-radius: 8px;
        background-color: rgba(225, 245, 255, 0.07);
        color: #ffffff;
        font-size: 1rem;
        appearance: none;
      }

      input[type="time"]:disabled,
      input[type="text"]:disabled,
      input[type="number"]:disabled,
      input[type="date"]:disabled {
        -webkit-text-fill-color: rgba(255, 255, 255, 0.25);
        color: rgba(255, 255, 255, 0.25);
      }

      input[type="submit"] {
        background-color: var(--accent-color);
        color: white;
        font-size: 1rem;
        border: none;
        border-radius: 999px;
        cursor: pointer;
        transition: background-color 0.2s ease-in-out;
        padding: 0.9rem 1.8rem;
      }

      input[type="submit"]:hover {
        background-color: var(--accent-color);
        filter: brightness(1.2);
      }

      input[type="time"]::-webkit-calendar-picker-indicator,
      input[type="date"]::-webkit-calendar-picker-indicator {
        filter: invert(100%);
      }

      .toggle-switch input[type="checkbox"]:checked + .toggle-slider {
        background-color: var(--accent-color);
      }

      .toggle-switch input[type="checkbox"]:disabled + .toggle-slider::before {
        background-color: rgba(204, 204, 204, 0.5);
      }

      input:-webkit-autofill,
      input:-webkit-autofill:hover,
      input:-webkit-autofill:focus,
      input:-webkit-autofill:active {
        -webkit-text-fill-color: white !important;
        -webkit-box-shadow: 0 0 0px 1000px rgba(225, 245, 255, 0.07) inset !important;
        box-shadow: 0 0 0px 1000px rgba(225, 245, 255, 0.07) inset !important;
        transition:
          background-color 5000000s ease-in-out 0s,
          color 5000000s ease-in-out 0s;
        caret-color: white;
      }

      input::placeholder,
      textarea::placeholder {
        color: hwb(0 100% 0% / 0.39);
        opacity: 1;
      }

      .form-row {
        display: flex;
        flex-direction: column;
      }

      .form-row.two-col {
        flex-direction: column;
      }

      .form-row.two-col > div {
        flex: 1;
      }

      .primary-button {
        background: var(--accent-color);
        color: white;
        padding: 0.9rem 1.8rem;
        font-size: 1rem;
        font-weight: 600;
        border: none;
        border-radius: 999px;
        cursor: pointer;
        text-align: center;
        align-self: center;
        width: fit-content;
        transition:
          background 0.25s,
          transform 0.15s ease-in-out;
      }

      @media (hover: hover) {
        .primary-button:hover {
          transform: translateY(-1px);
          box-shadow: 0 6px 16px rgba(0, 122, 255, 0.35);
          filter: brightness(1.2);
        }
      }

      .primary-button:active {
        transform: scale(0.97);
      }

      #customMessage {
        text-transform: uppercase;
      }

      .note {
        font-size: 0.85rem;
        text-align: center;
        margin-top: 1rem;
      }

      #savingModal {
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        display: none;
        justify-content: center;
        align-items: center;
        z-index: 1000;
        background: rgba(0, 0, 0, 0.45);
        z-index: 1000;
        transition: opacity 0.3s ease;
      }

      #savingModalContent {
        background: var(--glass-bg);
        border-radius: 24px;
        box-shadow:
          0 10px 36px 0 rgba(40, 170, 255, 0.11),
          0 2px 8px 0 rgba(44, 70, 110, 0.08);
        margin: 1.5rem;
        padding: 2rem 2.5rem;
        text-align: center;
        border: 1px solid var(--glass-border);
        backdrop-filter: blur(15px);
      }

      .spinner {
        border: 4px solid rgba(255, 255, 255, 0.2);
        border-top: 4px solid #007aff;
        border-radius: 50%;
        width: 36px;
        height: 36px;
        animation: spin 1s linear infinite;
        margin: 0 auto 1rem;
      }

      .footer {
        font-size: 0.8rem;
        text-align: center;
        margin-top: 1rem;
        opacity: 0.8;
      }

      a {
        color: white;
      }

      .small {
        display: block;
        font-size: 0.8rem;
        margin-top: 0.25rem;
      }

      select option {
        color: black;
      }

      .geo-disabled {
        opacity: 0.5;
        background: none !important;
        color: white !important;
        border: 0.1rem white solid;
        cursor: not-allowed;
      }

      .geo-disabled:hover {
        transform: none;
        box-shadow: none;
        background-color: transparent;
        background: none;
      }

      .button-row {
        display: flex;
        margin-top: 0.5rem;
        gap: 1rem;
        justify-content: space-evenly;
        flex-direction: column;
      }

      @keyframes spin {
        0% {
          transform: rotate(0deg);
        }

        100% {
          transform: rotate(360deg);
        }
      }

      #openWeatherCountry {
        margin-top: 0.75rem;
      }

      @media (min-width: 361px) {
        .form-row.two-col {
          flex-direction: row;
          gap: 1rem;
        }

        .button-row {
          flex-direction: row;
          gap: 0;
        }

        #openWeatherCountry {
          margin-top: 0;
        }
      }

      .toggle-switch {
        display: inline-flex;
        align-items: center;
        gap: 10px;
        cursor: pointer;
        position: relative;
      }

      .toggle-switch input {
        opacity: 0;
        width: 0;
        height: 0;
        position: absolute;
      }

      .toggle-slider {
        position: relative;
        width: 48px;
        height: 24px;
        border-radius: 999px;
        transition: all 0.18s cubic-bezier(0.4, 0, 0.2, 1);

        background: rgba(255, 255, 255, 0.2);
        box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.12);
      }

      .toggle-slider::before {
        content: "";
        position: absolute;
        height: 18px;
        width: 18px;
        left: 3px;
        top: 3px;
        border-radius: 50%;
        background: #ffffff;
        transition: all 0.18s cubic-bezier(0.4, 0, 0.2, 1);
        box-shadow: 0 2px 6px rgba(0, 0, 0, 0.4);
      }

      .toggle-switch input:checked + .toggle-slider {
        background: var(--accent);
        box-shadow: 0 0 10px rgba(0, 191, 255, 0.35);
      }

      .toggle-switch input:checked + .toggle-slider::before {
        transform: translateX(24px);
      }

      .toggle-switch input:disabled + .toggle-slider {
        opacity: 0.6;
        cursor: not-allowed;
        border: none;
        background: transparent;
      }

      .accent {
        accent-color: var(--accent-color);
      }

      .collapsible-toggle {
        display: flex;
        align-items: center;
        cursor: pointer;
        font-size: 1.1rem;
        font-weight: normal;
        background: none;
        border: none;
        color: white;
        padding: 0;
        margin: 0;
        outline: none;
        gap: 0.5em;
        user-select: none;
        margin-top: 6rem;
      }

      #configForm > button.collapsible-toggle > span:nth-child(2) {
        border-bottom: solid 1px white;
      }

      .collapsible-toggle .icon-area {
        transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        display: flex;
      }

      .collapsible-toggle.open .icon-area {
        transform: rotate(90deg);
      }

      .collapsible-content {
        overflow: hidden;
        height: 0;
        transition: height 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        color: #fff;
        margin-bottom: 3rem;
      }

      .collapsible-content-inner {
        padding: 1em 0;
      }

      input[type="range"] {
        -webkit-appearance: none;
        width: 100%;
        height: 6px;
        background: transparent;
        cursor: pointer;
      }

      input[type="range"]::-webkit-slider-runnable-track {
        height: 6px;
        background: rgba(255, 255, 255, 0.2);
        border-radius: 6px;
      }

      input[type="range"]::-webkit-slider-thumb {
        -webkit-appearance: none;
        width: 18px;
        height: 18px;
        border-radius: 50%;
        background: var(--accent-color);
        border: none;
        margin-top: -6px;
        box-shadow: 0 0 10px var(--accent-color);
        transition: transform 0.15s ease;
      }

      input[type="range"]:hover::-webkit-slider-thumb {
        transform: scale(1.1);
      }

      input[type="range"]::-moz-range-track {
        height: 6px;
        background: rgba(255, 255, 255, 0.2);
        border-radius: 6px;
      }

      input[type="range"]::-moz-range-thumb {
        width: 18px;
        height: 18px;
        border-radius: 50%;
        background: var(--accent-color);
        border: none;
        box-shadow: 0 0 10px var(--accent-color);
      }

      input[type="range"]:disabled {
        cursor: not-allowed;
        opacity: 0.6;
      }

      input[type="range"]:disabled::-webkit-slider-runnable-track {
        background: rgba(255, 255, 255, 0.08);
      }

      input[type="range"]:disabled::-moz-range-track {
        background: rgba(255, 255, 255, 0.08);
      }

      input[type="range"]:disabled::-webkit-slider-thumb {
        background: #777;
        box-shadow: none;
        transform: none;
      }

      input[type="range"]:disabled::-moz-range-thumb {
        background: #777;
        box-shadow: none;
      }

      .sub-collapsible {
        color: inherit;
        width: fit-content;
        background: none;
        border: none;
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 1.5rem 1.5rem 1rem 1rem;
        font-weight: 600;
        letter-spacing: 0.08em;
        text-transform: uppercase;
        opacity: 0.65;
        cursor: pointer;
        font-size: 0.9rem;
      }

      .sub-collapsible.open {
        opacity: 1;
      }

      .sub-collapsible-content {
        overflow: hidden;
        height: 0;
        transition: height 0.3s ease;
      }

      .sub-collapsible-content .content-wrapper {
        border-radius: 12px;
        padding: 0.5rem 1.5rem 2.5rem 1.5rem;
        background: var(--glass-bg);
        margin-bottom: 1.5rem;
      }

      #device {
        margin-bottom: 0;
      }

      .sub-collapsible[aria-expanded="true"] + .sub-collapsible-content {
        display: block;
      }

      .sub-collapsible::after {
        padding-bottom: 0.2rem;
        padding-left: 0.4rem;
        line-height: 0;
        font-size: 1.1rem;
        content: "›";
        transition: transform 0.25s ease;
      }

      .sub-collapsible.open::after {
        transform: translateY(-0.1rem) rotate(90deg);
        opacity: 1;
      }

      .content-wrapper > label:nth-child(1) {
        margin-top: 0;
      }

      #autoDimmingNote {
        opacity: 0.5;
        text-align: unset;
        position: absolute;
        padding-right: 3rem;
        top: 0.9rem;
      }

      .toggle-row {
        display: flex;
        align-items: center;
        justify-content: space-between;
      }

      .toggle-row-lg {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-top: 1.75rem;
      }

      .toggle-row-sm {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-top: 0.75rem;
      }

      .label-text {
        margin-right: 0.5em;
      }

      .range-full {
        width: 100%;
      }

      .mt-lg {
        margin-top: 1.75rem;
      }

      #configForm
        > div.collapsible-content
        > div:nth-child(8)
        > div
        > div.toggle-padding
        > div:nth-child(1)
        > label {
        margin-top: 0.75rem;
      }

      #configForm
        > div.collapsible-content
        > div:nth-child(6)
        > div
        > div.toggle-padding
        > label:nth-child(4) {
        position: relative;
      }

      #geo-button,
      .primary-button.cmsg1,
      .primary-button.cmsg2 {
        background-color: hsl(0deg 0% 100% / 25%);
        font-weight: normal;
        font-size: 0.75rem;
      }

      #geo-button{
        display: inline-flex;
        gap: 0.3rem;
        align-items: end;
      }

      @media (hover: hover) {
        #geo-button:hover,
        .primary-button.cmsg1:hover {
          background-color: var(--accent-color);
          color: white;
        }
      }

      @media (hover: hover) {
        .primary-button.cmsg2:hover {
          background-color: #ff3c3c;
          color: white;
        }
      }

      .device-info {
        margin-top: 0.75rem;
      }

      hr {
        margin: 1.5rem 0;
        border: 0;
        border-top: 1px solid var(--glass-border);
      }

      #hostnameInput {
        min-width: calc(3ch + 1rem);
        text-align: center;
        padding: 0.5rem;
      }

      .no-ap {
        display: none !important;
      }

      .no-st {
        display: none;
      }

      .logo path {
        fill: currentColor;
        paint-order: markers fill stroke;
      }

      .arrow-icon {
        opacity: 1;
      }

      .arrow-icon-bg {
        fill: none;
        stroke-width: 0.0127174;
        paint-order: markers fill stroke;
      }

      .arrow-icon-fg {
        fill: #fff;
        stroke-width: 0.0600023;
        stroke-linecap: round;
        stroke-linejoin: round;
      }

      .password-field-wrapper {
        position: relative;
      }

      .toggle-password-cb {
        margin-right: 0.3rem;
      }

      .geo-note {
        display: none;
      }

      .geo-note > .small {
        margin-top: 0.5rem;
      }

      #geo-button {
        margin-top: 1rem;
      }

      .loading-hint {
        opacity: 0.5;
      }

      .btn-apply-wrap {
        text-align: center;
      }

      .btn-apply-top {
        margin-top: 1.5rem;
        min-width: 120px;
      }

      #btn-save-status {
        text-align: center;
        margin-top: 0.5rem;
        min-height: 1.2em;
      }

      .btn-gpio-hints {
        font-size: 0.9rem;
        opacity: 0.75;
        margin: 0 0 1rem 0;
        line-height: 1.5;
      }

      .donation-note {
        font-size: 0.78rem;
        color: rgba(255, 255, 255, 0.45);
        margin: 1rem 0 2.5rem 0;
      }

      #ota-container {
        text-align: center;
      }

      #ota-update-found {
        display: none;
      }

      /* Dynamically generated button-config classes */
      .btn-add-button {
        background: none;
        border: 1px dashed rgba(255, 255, 255, 0.2);
        border-radius: 8px;
        width: 100%;
        padding: 0.6rem;
        color: rgba(255, 255, 255, 0.5);
        cursor: pointer;
        font-size: 0.9rem;
        margin-bottom: 0.75rem;
      }

      .btn-config-separator {
        border: 0;
        border-top: 1px solid rgba(255, 255, 255, 0.08);
        margin: 0 0 2.5rem;
      }

      .btn-config-row {
        margin-bottom: 1.5rem;
      }

      .btn-config-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-bottom: 0.5rem;
        margin-top: 1.5rem;
      }

      .btn-config-label {
        font-weight: 600;
        margin: 0;
      }

      .btn-config-remove {
        background: none;
        border: none;
        color: rgba(255, 255, 255, 0.35);
        cursor: pointer;
        font-size: 0.8rem;
      }

      .btn-config-gpio-label {
        margin-top: 0.5rem;
      }

      .btn-config-action-label {
        margin-top: 0.75rem;
      }

      .ota-progress-value {
        font-size: 1.35em;
        font-weight: bold;
        color: #2ecc71;
      }

      .select-and-toggle {
        margin: 0;
        gap: 2rem;
        align-items: flex-start;
      }

      .sep {
        margin-bottom: 3rem;
      }

      .alarm-days-row {
        display: flex;
        flex-wrap: wrap;
        gap: 0.8rem;
        margin-top: 0.5rem;
        justify-content: center;
        margin-bottom: 1.5rem;
      }

      .alarm-preview-text {
        opacity: 0.7;
        font-size: 0.85rem;
        margin: 0.75rem 0 0.25rem 0;
      }

      .day-pill {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 2.4rem;
        height: 2.4rem;
        border-radius: 999px;
        background: rgba(255, 255, 255, 0.07);
        border: 1.5px solid rgba(180, 230, 255, 0.08);
        cursor: pointer;
        font-size: 0.8rem;
        transition:
          background 0.2s,
          border-color 0.2s;
        user-select: none;
        margin-top: 0;
      }

      .day-pill input {
        display: none;
      }

      .day-pill.checked {
        background: var(--accent-color);
        border-color: var(--accent-color);
        color: white;
      }

      .day-pill.disabled {
        opacity: 0.4;
        cursor: not-allowed;
      }

      #alarmPreviewText {
        opacity: 0.7;
        font-size: 0.85rem;
        margin: 0.75rem 0 0.25rem 0;
        text-align: center;
      }

      #weather-save-status,
      #timedate-save-status,
      #display-save-status,
      #alarm-save-status,
      #countdown-save-status,
      #buzzer-save-status,
      #btn-save-status,
      #ota-status-text {
        text-align: center;
      }

      .language-row {
        display: flex;
        justify-content: space-between;
        gap: 1rem;
        align-items: end;
        margin-top: 0.3rem;
      }

      .language-row > div {
        display: flex;
        flex-direction: column;
      }

      .language-row select {
        flex: 1;
        min-width: 0;
      }

      .language-small {
        font-size: 0.7rem;
        color: rgba(255, 255, 255, 0.5);
      }

      .clockduration-row {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-top: 1.5rem;
      }

      .clockduration-row > div {
        display: flex;
        flex-direction: column;
        min-width: 13ch;
      }

      .clockduration-row input {
        flex: 1;
        min-width: 0;
      }

      .clockduration-row > div > label {
        margin-top: 0;
      }

      .clockduration-small {
        font-size: 0.7rem;
        color: rgba(255, 255, 255, 0.5);
      }

      .toggles.toggle-padding {
        margin-top: 2rem;
      }

      #weather > div.clockduration-row > div {
        min-width: 14ch;
      }

      #weather > div.toggle-padding {
        margin-top: 2rem;
      }

      #display > div.toggle-padding > label:nth-child(1) {
        margin-top: 1.5rem;
      }

      #buzzer > div:nth-child(1) > label.toggle-row-lg {
        margin-top: 1.5rem;
      }

      #buzzer > div:nth-child(1) > div.language-row {
        align-items: center;
        margin-top: 1.75rem;
      }

      #move-time-language > div {
        margin-top: 1.5rem;
      }

      span:has(#hostnameInput) {
        display: block;
        margin-top: 0.5rem;
      }

      #saveandreboot {
        margin-top: 3rem;
      }

      .spanholder {
        display: grid;
        gap: 0.5rem;
        color: rgba(255, 255, 255, 0.5);
        font-size: 0.9rem;
        margin-top: 1rem;
      }

      .quick-grid {
        display: grid;
        grid-template-columns: repeat(2, 1fr);
        gap: 10px;
        margin-bottom: 2rem;
      }

      .quick-section {
        border-radius: 8px;
        background: var(--glass-bg);
        border: 0;
        min-height: 82px;
        padding: 1rem;
        text-align: left;
        color: inherit;
      }

      .quick-buttons-section {
        display: grid;
        grid-auto-flow: column;
        gap: 0.5rem;
        grid-template-columns: repeat(2, 1fr);
      }

      .quick-buttons-section > button {
        background-color: hsl(0deg 0% 100% / 20%);
        border: none;
        color: white;
        font-size: 0.75rem;
        padding: 0.5rem;
        border-radius: 8px;
        cursor: pointer;
        transition: 0.15s;
      }

      @media (hover: hover) {
        .quick-buttons-section > button:hover {
        transform: translateY(-1px);
        box-shadow: 0 6px 16px rgba(0, 122, 255, 0.35);
        filter: brightness(1.2);
        background: var(--accent-color);
        }
      }

      .quick-section span {
        font-size: 0.9rem;
        margin-bottom: 0.5rem;
        display: flex;
        align-items: end;
        justify-content: center;
        gap: 0.2rem;
      }

      .quick:active {
        transform: scale(0.98);
      }

      .quick.active {
        background: #eef1ff;
        color: var(--accent);
      }

      .quick.danger {
        color: #c0392b;
      }

      .quick-timer {
        display: flex;
        align-items: center;
        gap: 13px;
        margin-top: 10px;
        padding: 13px;
        background: var(--card);
        border-radius: 17px;
        box-shadow: var(--shadow);
      }

      .quick-timer-icon {
        width: 34px;
        height: 34px;
        border-radius: 10px;
        background: #f0f2f6;
        display: grid;
        place-items: center;
        font-size: 17px;
        flex-shrink: 0;
      }

      .quick-timer-content {
        flex: 1;
        min-width: 0;
      }

      .quick-timer-content > b {
        display: block;
        font-size: 14px;
        margin-bottom: 8px;
      }

      .quick-timer-controls {
        display: flex;
        gap: 7px;
      }

      .quick-timer input {
        flex: 1;
        min-width: 0;
        border: 1px solid var(--line);
        background: #fafafa;
        border-radius: 10px;
        padding: 9px 10px;
        font: inherit;
      }

      .quick-timer button {
        border: 0;
        border-radius: 10px;
        background: var(--accent);
        color: white;
        padding: 0 12px;
        font: inherit;
        font-size: 13px;
        font-weight: 600;
        cursor: pointer;
      }

      .quick-timer button:last-child {
        background: #f0f2f6;
        color: var(--text);
      }

      .toast {
        position: fixed;
        left: 50%;
        bottom: 90px;
        transform: translateX(-50%) translateY(12px);
        background: #fff;
        color: #000;
        padding: 11px 17px;
        border-radius: 999px;
        font-size: 0.75rem;
        box-shadow: 0 6px 20px rgb(255 255 255 / 18%);
        opacity: 0;
        pointer-events: none;
        transition:
          opacity 0.2s ease,
          transform 0.2s ease;
        z-index: 9999;
        white-space: nowrap;
      }

      .toast.show {
        opacity: 1;
        transform: translateX(-50%) translateY(0);
      }

      #weather-city{
        text-transform: capitalize;
      }

      #weather-settings{
        scroll-margin-top: 3.5vh;
      }

      @media (max-width: 500px) {
        .toast {
          bottom: 85px;
          max-width: calc(100% - 32px);
          white-space: normal;
          text-align: center;
        }
      }

      @media (max-width: 430px) {
        .quick {
          min-height: 78px;
        }

        .quick-timer-controls {
          flex-wrap: wrap;
        }

        .quick-timer input {
          width: 100%;
          flex-basis: 100%;
        }

        .quick-timer button {
          flex: 1;
          min-height: 38px;
        }
      }

      #quickactions {
        margin: 3vh 0 8vh 0;
      }

      @media (max-width: 360px) {
        .timer-quick-row {
          flex-wrap: wrap;
        }

        .timer-quick-row input {
          width: 100%;
          flex-basis: 100%;
        }
      }

      #message-buttons {
        margin-top: 1rem;
      }

      @media (max-width: 500px) {
        /* mobile/small-screen changes */

        form {
          background: none;
          border: none;
          border-radius: unset;
          box-shadow: none;
        }
      }
    </style>
  </head>
  <body>
    <form id="configForm" onsubmit="submitConfig(event)">
      <div class="logo">
        <svg
          width="450"
          height="50"
          viewBox="0 0 119.063 13.229"
          xml:space="preserve"
          xmlns="http://www.w3.org/2000/svg"
        >
          <path
            class="logo-path"
            d="M.75 0a.75.75 0 1 0-.002 1.495A.75.75 0 0 0 .75 0m1.957 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.082 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0-.002 1.495A.75.75 0 0 0 24.654 0m1.957 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m1.956 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.126 0a.75.75 0 1 0-.002 1.495A.75.75 0 0 0 48.558 0m29.987 0a.75.75 0 1 0-.002 1.495A.75.75 0 0 0 78.545 0M80.5 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498M114.4 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498M.75 1.955a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .002-1.5m11.952 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m7.826 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 .001-1.498m4.126 0a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .002-1.5m7.826 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 .001-1.498m8.039 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m36.069 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m7.826 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m29.986 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498M.75 3.91a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m11.952 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m11.952 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m7.826 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.039 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.04 0a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .001-1.5m4.125 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.039 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m13.908 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 .001-1.5m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.125 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498M.75 5.866a.75.75 0 1 0-.002 1.499.75.75 0 0 0 .002-1.5m1.957 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m1.956 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m1.956 0a.749.749 0 1 0 0 1.497.75.75 0 1 0 0-1.498m8.039 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m1.957 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m1.956 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m6.083 0a.75.75 0 1 0-.002 1.499.75.75 0 0 0 .002-1.5m1.957 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m1.956 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 .001-1.498m1.956 0a.749.749 0 1 0 0 1.497.75.75 0 1 0 0-1.498m9.996 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m8.04 0a.75.75 0 1 0-.003 1.499.75.75 0 0 0 .002-1.5m4.125 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m3.913 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m3.913 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m4.125 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m7.827 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m4.126 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m11.951 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m7.827 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m4.126 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m13.908 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497M.75 7.82a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m19.778 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.126 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m15.865 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.04 0a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .001-1.5m4.125 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.5m3.913 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m3.913 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.125 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.126 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.5m11.951 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m7.827 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.5m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 .001-1.5m8.038 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498M.75 9.776a.75.75 0 1 0-.002 1.499.75.75 0 0 0 .002-1.499m11.952 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m7.826 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m4.126 0a.75.75 0 1 0-.002 1.499.75.75 0 0 0 .002-1.499m15.865 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m8.04 0a.75.75 0 1 0-.003 1.499.75.75 0 0 0 .002-1.499m4.125 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m3.913 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m3.913 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m4.125 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m11.953 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m7.826 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m4.125 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m5.87 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m1.957 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m11.952 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m6.082 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497M.75 11.731a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m1.957 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.082 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m15.865 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.04 0a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .001-1.5m4.125 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m3.913 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m3.913 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.082 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.04 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.039 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m3.913 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.126 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.499m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 .001-1.5m9.995 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498"
          />
        </svg>
        <div id="device-status" class="no-ap">
          <div id="weather-status">
            <span id="weather-city"></span>
            <span id="weather-icon"></span>
            <span id="weather-temp"></span>
            <span id="weather-condition"></span>
          </div>
        </div>
      </div>
      <h2 class="no-st">Wi-Fi Settings</h2>
      <div id="move-wifi">
        <label for="ssid">Enter Wi-Fi network or click Scan</label>
        <div class="ssid-wrapper">
          <div class="combo-container">
            <input type="text" id="ssid" name="ssid" required />
            <button type="button" id="arrowBtn" title="Show scanned" disabled>
              <svg
                xmlns="http://www.w3.org/2000/svg"
                width="16"
                height="16"
                fill="none"
                viewBox="0 0 0.48 0.48"
              >
                <g class="arrow-icon">
                  <path
                    d="M-24.679 12.619h.48v.48h-.48z"
                    class="arrow-icon-bg"
                    transform="translate(24.679 -12.62)"
                  />
                  <path
                    d="M-24.661 12.732a.06.06 0 0 0 0 .085l.162.162a.086.086 0 0 0 .12 0l.163-.162a.06.06 0 0 0 0-.085.06.06 0 0 0-.085 0l-.138.138-.137-.138a.06.06 0 0 0-.085 0z"
                    class="arrow-icon-fg"
                    transform="translate(24.679 -12.62)"
                  />
                </g>
              </svg>
            </button>
            <button type="button" class="primary-button" id="scanBtn">
              Scan
            </button>
          </div>
          <div id="ssidList"></div>
        </div>
        <label for="password">Enter your Wi-Fi password</label>
        <div class="password-field-wrapper">
          <input type="password" id="password" name="password" />
          <label class="small">
            <input
              type="checkbox"
              id="togglePassword"
              class="toggle-password-cb"
            />
            Show Password
          </label>
        </div>
      </div>

      <div id="move-time-language">
        <div class="language-row">
          <div>
            <label for="language">Language:</label>
            <div class="language-small">Date & Weather</div>
          </div>
          <select
            id="language"
            name="language"
            onchange="setLanguage(this.value)"
          >
            <option value="" disabled selected>Select language</option>
            <option value="af">Afrikaans</option>
            <option value="hr">Croatian</option>
            <option value="cs">Czech</option>
            <option value="da">Danish</option>
            <option value="nl">Dutch</option>
            <option value="en">English</option>
            <option value="eo">Esperanto</option>
            <option value="et">Estonian</option>
            <option value="fi">Finnish</option>
            <option value="fr">French</option>
            <option value="de">German</option>
            <option value="hu">Hungarian</option>
            <option value="it">Italian</option>
            <option value="ga">Irish</option>
            <option value="ja">Japanese</option>
            <option value="lv">Latvian</option>
            <option value="lt">Lithuanian</option>
            <option value="no">Norwegian</option>
            <option value="pl">Polish</option>
            <option value="pt">Portuguese</option>
            <option value="ro">Romanian</option>
            <option value="ru">Russian</option>
            <option value="sr">Serbian</option>
            <option value="sk">Slovak</option>
            <option value="sl">Slovenian</option>
            <option value="es">Spanish</option>
            <option value="sv">Swedish</option>
            <option value="sw">Swahili</option>
            <option value="tr">Turkish</option>
          </select>
        </div>
      </div>
      <div id="quickactions" class="no-ap">
        <h2>Quick Actions</h2>
        <div class="quick-grid">
          <div class="quick-section">
            <span
              ><svg
                xmlns="http://www.w3.org/2000/svg"
                width="24"
                height="24"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"
                class="lucide lucide-timer-icon lucide-timer"
              >
                <line x1="10" x2="14" y1="2" y2="2" />
                <line x1="12" x2="15" y1="14" y2="11" />
                <circle cx="12" cy="14" r="8" /></svg
              >STOPWATCH</span
            >
            <div class="quick-buttons-section">
              <button type="button" onclick="quickAction('stopwatch')">
                Start
              </button>
              <button type="button" onclick="quickAction('stopwatch_clear')">
                Clear
              </button>
            </div>
          </div>

          <div class="quick-section">
            <span
              ><svg
                xmlns="http://www.w3.org/2000/svg"
                width="24"
                height="24"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"
                class="lucide lucide-alarm-clock-check-icon lucide-alarm-clock-check"
              >
                <circle cx="12" cy="13" r="8" />
                <path d="M5 3 2 6" />
                <path d="m22 6-3-3" />
                <path d="M6.38 18.7 4 21" />
                <path d="M17.64 18.67 20 21" />
                <path d="m9 13 2 2 4-4" /></svg
              >POMODORO</span
            >
            <div class="quick-buttons-section">
              <button type="button" onclick="quickAction('pomodoro_start')">
                Start
              </button>
              <button type="button" onclick="quickAction('pomodoro_stop')">
                Stop
              </button>
            </div>
          </div>

          <div class="quick-section">
            <span
              ><svg
                xmlns="http://www.w3.org/2000/svg"
                width="24"
                height="24"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"
                class="lucide lucide-bell-ring-icon lucide-bell-ring"
              >
                <path d="M10.268 21a2 2 0 0 0 3.464 0" />
                <path d="M22 8c0-2.3-.8-4.3-2-6" />
                <path
                  d="M3.262 15.326A1 1 0 0 0 4 17h16a1 1 0 0 0 .74-1.673C19.41 13.956 18 12.499 18 8A6 6 0 0 0 6 8c0 4.499-1.411 5.956-2.738 7.326"
                />
                <path d="M4 2C2.8 3.7 2 5.7 2 8" /></svg
              >ALARM</span
            >
            <div class="quick-buttons-section">
              <button type="button" onclick="quickAction('alarm_stop')">
                Stop
              </button>
              <button type="button" onclick="quickAction('alarm_snooze')">
                Snooze
              </button>
            </div>
          </div>

          <div class="quick-section">
            <span
              ><svg
                xmlns="http://www.w3.org/2000/svg"
                width="24"
                height="24"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"
                class="lucide lucide-refresh-cw-icon lucide-refresh-cw"
              >
                <path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8" />
                <path d="M21 3v5h-5" />
                <path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16" />
                <path d="M8 16H3v5" /></svg
              >MODE</span
            >
            <div class="quick-buttons-section">
              <button type="button" onclick="quickAction('enable_rotation')">
                Pause
              </button>
              <button type="button" onclick="quickAction('next_mode')">
                Next
              </button>
            </div>
          </div>
        </div>

        <div class="form-group no-ap">
          <h2>Custom Message</h2>
          <input
            id="customMessage"
            name="customMessage"
            type="text"
            maxlength="120"
            placeholder="ENTER MESSAGE"
            title="Allowed: A-Z, 0-9, space, and symbols : ! ' . , _ + % / ? [ ] ° # @ ^ ~ * = < > ( ) { } \ - & $ ¥ |"
          />
          <div class="small">
            <strong>Use brackets for special content:</strong> [123] for Big
            Numbers, icons like [CAR] or [MAIL], [TIMER 5M] for a 5-minute
            timer. Full icon list
            <a
              href="https://github.com/mfactory-osaka/ESPTimeCast#-using-mfactoryfonth-icons-v123"
              target="_blank"
              >here</a
            >.
          </div>
        </div>

        <div class="button-row no-ap" id="message-buttons">
          <button
            type="button"
            class="primary-button cmsg2"
            onclick="clearCustomMessage()"
          >
            Clear Message
          </button>
          <button
            type="button"
            class="primary-button cmsg1"
            onclick="sendCustomMessage()"
          >
            Send Message
          </button>
        </div>
      </div>

      <div class="geo-note">
        <span class="small">
          Enter your Wi-Fi details and press “Save Settings” to unlock Weather,
          Custom Messages, and Advanced Features.
        </span>
      </div>
      <button
        id="weather-settings"
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        Weather Settings
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="weather">
          <div class="clockduration-row">
            <div>
              <label for="weatherDuration">Weather Duration:</label>
              <span class="clockduration-small">Seconds</span>
            </div>
            <input
              type="number"
              id="weatherDuration"
              name="weatherDuration"
              min="1"
            />
          </div>
          <label class="no-ap" for="openWeatherApiKey"
            >OpenWeather API Key:</label
          >
          <input
            class="no-ap"
            type="text"
            id="openWeatherApiKey"
            name="openWeatherApiKey"
            placeholder="ADD-YOUR-API-KEY-32-CHARACTERS"
          />
          <div class="small no-ap">
            Required to fetch weather data.
            <a
              href="https://home.openweathermap.org/users/sign_up"
              target="_blank"
              >Get your API key here</a
            >.
          </div>
          <label class="no-ap">Location:</label>
          <div class="form-row two-col no-ap">
            <input
              type="text"
              id="openWeatherCity"
              name="openWeatherCity"
              placeholder="City / Zip / Lat."
            />
            <input
              type="text"
              id="openWeatherCountry"
              name="openWeatherCountry"
              placeholder="Country Code / Long."
            />
          </div>

          <div class="small no-ap">
            <strong>Location format examples:</strong> City, Country Code -
            Osaka, JP | ZIP, Country Code - 94040, US | Latitude, Longitude -
            34.6937, 135.5023
          </div>

          <div class="btn-apply-wrap">
            <button
              type="button"
              class="primary-button no-ap"
              id="geo-button"
              onclick="getLocation()"
            >
              Get My Location<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-external-link-icon lucide-external-link"><path d="M15 3h6v6"/><path d="M10 14 21 3"/><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/></svg>
            </button>
          </div>

          <div class="toggle-padding">
            <label class="toggle-row-lg">
              <span class="label-text">Use Imperial Units (°F):</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="weatherUnits"
                  name="weatherUnits"
                  onchange="setWeatherUnits(this.checked)"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <label class="toggle-row-lg">
              <span class="label-text">Show Humidity:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="showHumidity"
                  name="showHumidity"
                  onchange="setShowHumidity(this.checked)"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <label class="toggle-row-lg">
              <span class="label-text">Show Weather Description:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="showWeatherDescription"
                  name="showWeatherDescription"
                  onchange="setShowWeatherDescription(this.checked)"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>
          </div>
          <div class="btn-apply-wrap">
            <button
              type="button"
              class="primary-button cmsg1 btn-apply-top"
              onclick="saveWeatherConfig()"
            >
              Apply
            </button>
          </div>
          <p id="weather-save-status"></p>
        </div>
      </div>

      <button
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        Time & Date
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="timeanddate">
          <div class="clockduration-row">
            <div>
              <label class="no-ap" for="timeZone">Time Zone:</label>
            </div>
            <select class="no-ap" id="timeZone" name="timeZone" required>
              <option value="" disabled selected>Select your time zone</option>
              <option value="Africa/Cairo">Africa/Cairo</option>
              <option value="Africa/Casablanca">Africa/Casablanca</option>
              <option value="Africa/Johannesburg">Africa/Johannesburg</option>
              <option value="America/Anchorage">America/Anchorage</option>
              <option value="America/Argentina/Buenos_Aires">
                America/Argentina/Buenos_Aires
              </option>
              <option value="America/Chicago">America/Chicago</option>
              <option value="America/Denver">America/Denver</option>
              <option value="America/Guatemala">America/Guatemala</option>
              <option value="America/Halifax">America/Halifax</option>
              <option value="America/Los_Angeles">America/Los_Angeles</option>
              <option value="America/Mexico_City">America/Mexico_City</option>
              <option value="America/New_York">America/New_York</option>
              <option value="America/Phoenix">America/Phoenix</option>
              <option value="America/Santiago">America/Santiago</option>
              <option value="America/Sao_Paulo">America/Sao_Paulo</option>
              <option value="America/St_Johns">America/St_Johns</option>
              <option value="America/Toronto">America/Toronto</option>
              <option value="America/Vancouver">America/Vancouver</option>
              <option value="Asia/Almaty">Asia/Almaty</option>
              <option value="Asia/Amman">Asia/Amman</option>
              <option value="Asia/Baghdad">Asia/Baghdad</option>
              <option value="Asia/Bangkok">Asia/Bangkok</option>
              <option value="Asia/Beirut">Asia/Beirut</option>
              <option value="Asia/Dhaka">Asia/Dhaka</option>
              <option value="Asia/Dubai">Asia/Dubai</option>
              <option value="Asia/Ho_Chi_Minh">Asia/Ho_Chi_Minh</option>
              <option value="Asia/Hong_Kong">Asia/Hong_Kong</option>
              <option value="Asia/Jakarta">Asia/Jakarta</option>
              <option value="Asia/Jerusalem">Asia/Jerusalem</option>
              <option value="Asia/Karachi">Asia/Karachi</option>
              <option value="Asia/Kathmandu">Asia/Kathmandu</option>
              <option value="Asia/Kolkata">Asia/Kolkata</option>
              <option value="Asia/Kuala_Lumpur">Asia/Kuala_Lumpur</option>
              <option value="Asia/Manila">Asia/Manila</option>
              <option value="Asia/Seoul">Asia/Seoul</option>
              <option value="Asia/Shanghai">Asia/Shanghai</option>
              <option value="Asia/Singapore">Asia/Singapore</option>
              <option value="Asia/Taipei">Asia/Taipei</option>
              <option value="Asia/Tashkent">Asia/Tashkent</option>
              <option value="Asia/Tehran">Asia/Tehran</option>
              <option value="Asia/Tokyo">Asia/Tokyo</option>
              <option value="Asia/Ulaanbaatar">Asia/Ulaanbaatar</option>
              <option value="Asia/Yekaterinburg">Asia/Yekaterinburg</option>
              <option value="Atlantic/Azores">Atlantic/Azores</option>
              <option value="Atlantic/Canary">Atlantic/Canary</option>
              <option value="Atlantic/Reykjavik">Atlantic/Reykjavik</option>
              <option value="Australia/Adelaide">Australia/Adelaide</option>
              <option value="Australia/Brisbane">Australia/Brisbane</option>
              <option value="Australia/Darwin">Australia/Darwin</option>
              <option value="Australia/Hobart">Australia/Hobart</option>
              <option value="Australia/Melbourne">Australia/Melbourne</option>
              <option value="Australia/Perth">Australia/Perth</option>
              <option value="Australia/Sydney">Australia/Sydney</option>
              <option value="Europe/Amsterdam">Europe/Amsterdam</option>
              <option value="Europe/Athens">Europe/Athens</option>
              <option value="Europe/Belgrade">Europe/Belgrade</option>
              <option value="Europe/Berlin">Europe/Berlin</option>
              <option value="Europe/Brussels">Europe/Brussels</option>
              <option value="Europe/Bucharest">Europe/Bucharest</option>
              <option value="Europe/Copenhagen">Europe/Copenhagen</option>
              <option value="Europe/Dublin">Europe/Dublin</option>
              <option value="Europe/Helsinki">Europe/Helsinki</option>
              <option value="Europe/Istanbul">Europe/Istanbul</option>
              <option value="Europe/Kiev">Europe/Kiev</option>
              <option value="Europe/Lisbon">Europe/Lisbon</option>
              <option value="Europe/London">Europe/London</option>
              <option value="Europe/Madrid">Europe/Madrid</option>
              <option value="Europe/Moscow">Europe/Moscow</option>
              <option value="Europe/Oslo">Europe/Oslo</option>
              <option value="Europe/Paris">Europe/Paris</option>
              <option value="Europe/Prague">Europe/Prague</option>
              <option value="Europe/Rome">Europe/Rome</option>
              <option value="Europe/Stockholm">Europe/Stockholm</option>
              <option value="Europe/Warsaw">Europe/Warsaw</option>
              <option value="Pacific/Auckland">Pacific/Auckland</option>
              <option value="Pacific/Chatham">Pacific/Chatham</option>
              <option value="Pacific/Fiji">Pacific/Fiji</option>
              <option value="Pacific/Guam">Pacific/Guam</option>
              <option value="Pacific/Honolulu">Pacific/Honolulu</option>
              <option value="Pacific/Port_Moresby">Pacific/Port_Moresby</option>
              <option value="Pacific/Tahiti">Pacific/Tahiti</option>
              <option value="UTC">UTC</option>
              <option value="Etc/GMT+1">Etc/GMT+1</option>
              <option value="Etc/GMT-1">Etc/GMT-1</option>
            </select>
          </div>

          <div class="clockduration-row">
            <div>
              <label for="clockDuration">Clock Duration:</label>
              <div class="clockduration-small">Seconds</div>
            </div>
            <input
              type="number"
              id="clockDuration"
              name="clockDuration"
              min="1"
            />
          </div>

          <div class="clockduration-row">
            <div>
              <label>Primary NTP:</label>
            </div>
            <input
              type="text"
              name="ntpServer1"
              id="ntpServer1"
              placeholder="Enter NTP address"
            />
          </div>

          <div class="clockduration-row">
            <div>
              <label>Secondary NTP:</label>
              <div class="clockduration-small">Integration URL</div>
            </div>
            <input
              type="text"
              name="ntpServer2"
              id="ntpServer2"
              placeholder="Enter URL"
            />
          </div>
          <div class="small">
            Enter a Nightscout URL, YouTube URL, RSS Feed URL, Instagram URL or
            secondary NTP server.
          </div>
          <div class="toggles toggle-padding">
            <label class="toggle-row-lg">
              <span class="label-text">Show Day Of The Week:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="showDayOfWeek"
                  name="showDayOfWeek"
                  onchange="setShowDayOfWeek(this.checked)"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <label class="toggle-row-lg">
              <span class="label-text">Animated Seconds:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="colonBlinkEnabled"
                  name="colonBlinkEnabled"
                  onchange="setColonBlink(this.checked)"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <label class="toggle-row-lg">
              <span class="label-text">Show Date:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="showDate"
                  name="showDate"
                  onchange="setShowDate(this.checked)"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <label class="toggle-row-lg">
              <span class="label-text">Display 12-hour Clock:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="twelveHourToggle"
                  name="twelveHourToggle"
                  onchange="setTwelveHour(this.checked)"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>
          </div>
          <div class="btn-apply-wrap">
            <button
              type="button"
              class="primary-button cmsg1 btn-apply-top"
              onclick="saveTimeDateConfig()"
            >
              Apply
            </button>
          </div>
          <p id="timedate-save-status"></p>
        </div>
      </div>

      <button
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        Display
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="display">
          <div class="toggle-padding">
            <label> Brightness: <span id="brightnessValue">10</span> </label>
            <input
              class="range-full"
              type="range"
              min="-1"
              max="15"
              name="brightness"
              id="brightnessSlider"
              value="10"
              oninput="
                brightnessValue.textContent =
                  this.value == -1 ? 'Off' : this.value;
                setBrightnessLive(this.value);
              "
            />

            <label class="toggle-row-lg">
              <span class="label-text">Flip Display (180°):</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="flipDisplay"
                  name="flipDisplay"
                  onchange="setFlipDisplay(this.checked)"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <label class="toggle-row-lg">
              <span class="label-text">Automatic Dimming:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="autoDimmingEnabled"
                  name="autoDimmingEnabled"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <label class="toggle-row-lg">
              <span class="label-text">Custom Dimming:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="dimmingEnabled"
                  name="dimmingEnabled"
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <label class="toggle-row-lg">
              <span class="label-text">Clock-Only Mode When Dimmed:</span>
              <span class="toggle-switch">
                <input type="checkbox" id="clockOnlyDuringDimming" />
                <span class="toggle-slider"></span>
              </span>
            </label>
          </div>

          <div class="form-row two-col">
            <div>
              <label for="dimStartTime">Start Time:</label>
              <input type="time" id="dimStartTime" value="18:00" />
            </div>

            <div>
              <label for="dimEndTime">End Time:</label>
              <input type="time" id="dimEndTime" value="08:00" />
            </div>
          </div>

          <label class="mt-lg" for="dimBrightness">
            Dimming Brightness: <span id="dimmingBrightnessValue">2</span>
          </label>

          <input
            class="range-full"
            type="range"
            min="-1"
            max="15"
            name="dimming_brightness"
            id="dimBrightness"
            value="2"
            oninput="
              dimmingBrightnessValue.textContent =
                this.value == -1 ? 'Off' : this.value
            "
          />

          <div class="btn-apply-wrap">
            <button
              type="button"
              class="primary-button cmsg1 btn-apply-top"
              onclick="saveDisplayConfig()"
            >
              Apply
            </button>
          </div>
          <p id="display-save-status"></p>
        </div>
      </div>

      <button
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        Alarm
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="alarm">
          <div id="alarm-config-container">
            <p class="small loading-hint">Loading...</p>
          </div>
          <div class="btn-apply-wrap">
            <button
              type="button"
              class="primary-button cmsg1 btn-apply-top"
              onclick="saveAlarmConfig()"
            >
              Apply
            </button>
          </div>
          <p id="alarm-save-status"></p>
        </div>
      </div>

      <button
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        Countdown
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="countdown">
          <div class="toggle-padding">
            <div class="form-group">
              <label class="toggle-row-lg">
                <span class="label-text">Enable Countdown:</span>
                <span class="toggle-switch">
                  <input
                    type="checkbox"
                    id="countdownEnabled"
                    name="countdownEnabled"
                  />
                  <span class="toggle-slider"></span>
                </span>
              </label>
            </div>

            <div class="form-group">
              <label class="toggle-row-lg">
                <span class="label-text">Dramatic Countdown:</span>
                <span class="toggle-switch">
                  <input
                    type="checkbox"
                    id="isDramaticCountdown"
                    name="isDramaticCountdown"
                  />
                  <span class="toggle-slider"></span>
                </span>
              </label>
            </div>
          </div>

          <div class="form-row two-col">
            <div class="form-group">
              <label for="countdownDate">Date:</label>
              <input
                type="date"
                id="countdownDate"
                name="countdownDate"
                class="form-control"
              />
            </div>
            <div class="form-group">
              <label for="countdownTime">Time:</label>
              <input
                type="time"
                id="countdownTime"
                name="countdownTime"
                class="form-control"
              />
            </div>
          </div>

          <div class="form-group">
            <label for="countdownLabel">Countdown Label (Optional):</label>
            <input
              type="text"
              id="countdownLabel"
              name="countdownLabel"
              class="form-control"
              placeholder="e.g., BIRTHDAY, VACATION"
              maxlength="24"
              pattern="[A-Z0-9 :!'\-.,_\+%\/?]*"
              title="Only uppercase letters, numbers, space, and : ! ' - . , _ + % / ? allowed"
            />
            <div class="small">
              Allowed characters: A–Z, 0–9, space, and : ! ' - . ? , _ + % /
            </div>
          </div>
          <div class="btn-apply-wrap">
            <button
              id="countdownApplyBtn"
              type="button"
              class="primary-button cmsg1 btn-apply-top"
              onclick="saveCountdownConfig()"
            >
              Apply
            </button>
          </div>
          <p id="countdown-save-status"></p>
        </div>
      </div>

      <button
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        Buzzer
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="buzzer">
          <div class="toggle-padding">
            <label class="toggle-row-lg">
              <span class="label-text">Enable Buzzer:</span>
              <span class="toggle-switch">
                <input
                  type="checkbox"
                  id="buzzerEnabled"
                  onchange="
                    setBuzzerFieldsEnabled(this.checked);
                    saveBuzzerConfig(false);
                  "
                />
                <span class="toggle-slider"></span>
              </span>
            </label>

            <div class="language-row">
              <div>
                <label class="btn-config-gpio-label" for="buzzerPin"
                  >GPIO Pin:</label
                >
              </div>
              <select
                id="buzzerPin"
                onchange="saveBuzzerConfig(false)"
              ></select>
            </div>

            <label>Volume: <span id="buzzerVolumeValue">7</span></label>
            <input
              class="range-full"
              type="range"
              min="1"
              max="10"
              id="buzzerVolumeSlider"
              value="7"
              oninput="buzzerVolumeValue.textContent = this.value"
            />

            <div class="btn-apply-wrap">
              <button
                type="button"
                id="buzzerTestBtn"
                class="primary-button cmsg1 btn-apply-top sep"
                onclick="testBuzzer()"
              >
                Test
              </button>
            </div>
          </div>

          <div id="buzzer-events-container" class="toggle-padding">
            <p class="small loading-hint">Loading...</p>
          </div>

          <div class="btn-apply-wrap">
            <button
              id="buzzerApplyBtn"
              type="button"
              class="primary-button cmsg1 btn-apply-top"
              onclick="saveBuzzerConfig()"
            >
              Apply
            </button>
          </div>
          <p id="buzzer-save-status"></p>
        </div>
      </div>

      <button
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        Buttons
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="buttons">
          <div id="btn-config-container">
            <p class="small loading-hint">Loading...</p>
          </div>
          <div class="btn-apply-wrap">
            <button
              type="button"
              class="primary-button cmsg1 btn-apply-top"
              onclick="saveButtonConfig()"
            >
              Apply
            </button>
          </div>
          <p id="btn-save-status"></p>
          <div class="btn-gpio-hints">
            • GPIO pins already used by ESPTimeCast are hidden automatically.<br />
            • Buttons should connect the selected GPIO pin to GND when pressed
            (no external resistor required).<br />
            • Select <em>- Disable -</em> if you do not want to use a button.
          </div>
        </div>
      </div>
      <button
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        OTA Update
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="otaupdate">
          <div class="toggle-padding device-info" id="ota-settings">
            <label class="toggle-row-lg donation-toggle-row">
              <span class="label-text">Already supporting ESPTimeCast ❤️:</span>
              <span class="toggle-switch">
                <input type="checkbox" id="hideDonationMsg" />
                <span class="toggle-slider"></span>
              </span>
            </label>
            <p class="donation-note">
              Thanks for supporting the project!<br />
              Turn this on if you'd rather keep those occasional encouragement
              messages off your display.
            </p>
            <div id="ota-container">
              <button
                type="button"
                id="btn-check-ota"
                onclick="checkUpdate()"
                class="primary-button cmsg1"
              >
                Check for Updates
              </button>
              <div id="ota-update-found">
                <button
                  type="button"
                  onclick="performUpdate()"
                  class="primary-button cmsg1"
                >
                  Install Update
                </button>
              </div>
              <p id="ota-status-text"></p>
            </div>
          </div>
        </div>
      </div>

      <button
        type="button"
        class="sub-collapsible no-ap active"
        aria-expanded="false"
      >
        Device settings
      </button>
      <div class="sub-collapsible-content no-ap" aria-hidden="true">
        <div class="content-wrapper" id="device">
          <div class="toggle-padding device-info" id="device-settings">
            <span
              >Host:
              <input
                type="text"
                id="hostnameInput"
                oninput="
                  this.value = this.value.replace(/[^a-zA-Z0-9-]/g, '');
                  resizeHostname(this);
                "
              />.local</span
            >
            <div class="spanholder">
              <span>IP: <span id="ipDisplay">Fetching...</span></span>
              <span>Firmware: <span id="fwVersion">...</span></span>
              <span
                >Session Uptime:
                <span id="sessionDisplay">Loading...</span></span
              >
              <span
                >Total Lifetime: <span id="totalDisplay">Loading...</span></span
              >
            </div>
          </div>
          <div class="btn-apply-wrap" id="saveandreboot">
            <input type="submit" class="primary-button" value="Save & Reboot" />
          </div>
        </div>
      </div>
      <div class="footer no-ap">
        <a
          href="https://esptimecast.com"
          target="_blank"
          rel="noopener noreferrer"
        >
          ESPTimeCast<span class="tm">™</span> by M-Factory
        </a>
      </div>

      <div id="savingMessage"></div>
      <div id="toast" class="toast"></div>
    </form>

    <script>
      let isSaving = false;
      let isAPMode = false;
      let pendingBinUrl = null;
      let pendingFromVersion = null;
      let deviceIP = "";
      const safeRegex = /[^A-Z0-9 #&¥$|°@^~*=<h3>(){}!.:?,'_+%\/\[\]\\-]/g;
      let originalHostname = "";

      // Show/Hide Password toggle
      document.addEventListener("DOMContentLoaded", function () {
        const passwordInput = document.getElementById("password");
        const toggleCheckbox = document.getElementById("togglePassword");

        toggleCheckbox.addEventListener("change", function () {
          if (this.checked) {
            // Show password as text
            passwordInput.type = "text";

            // Only clear if it's the masked placeholder
            if (passwordInput.value === "********") {
              passwordInput.value = "";
              passwordInput.placeholder = "Enter new password";
            }
          } else {
            // Hide password as dots
            passwordInput.type = "password";

            // Remove placeholder only if it was set by show-password toggle
            if (passwordInput.placeholder === "Enter new password") {
              passwordInput.placeholder = "";
            }
          }
        });
      });

      window.onbeforeunload = function () {
        if (isSaving) {
          return "Settings are being saved. Leaving now may interrupt the process.";
        }
      };

      (function () {
        const sel = document.getElementById("timeZone");
        if (!sel) return;
        try {
          const tz = Intl.DateTimeFormat().resolvedOptions().timeZone;
          sel.value = tz && sel.querySelector(`[value="${tz}"]`) ? tz : "UTC";
        } catch (e) {
          sel.value = "UTC";
        }
      })();

      window.onload = function () {
        fetch("/ap_status")
          .then((r) => r.json())
          .then((apStatus) => {
            isAPMode = apStatus.isAP;
            if (isAPMode) {
              document.querySelector(".geo-note").style.display = "block";
              document
                .getElementById("geo-button")
                .classList.add("geo-disabled");
              document.getElementById("geo-button").disabled = true;
              document.querySelector(".cmsg1").classList.add("geo-disabled");
              document.querySelector(".cmsg1").disabled = true;
              document.querySelector(".cmsg2").classList.add("geo-disabled");
              document.querySelector(".cmsg2").disabled = true;
              document.querySelector(
                "#move-time-language > div",
              ).style.marginTop = "0.5rem";

              const saveAndRebootDiv = document.getElementById("saveandreboot");
              const targetForm = document.getElementById("configForm");

              if (saveAndRebootDiv && targetForm) {
                targetForm.appendChild(saveAndRebootDiv);
              }

              document.querySelectorAll(".no-st").forEach((el) => {
                el.classList.remove("no-st");
              });
            }
            if (!isAPMode) {
              document.querySelectorAll(".no-ap").forEach((el) => {
                el.classList.remove("no-ap");
              });
              // Target the container

              const deviceSettings = document.getElementById("device-settings");
              const moveTimeLang =
                document.getElementById("move-time-language");
              const moveWifi = document.getElementById("move-wifi");

              if (deviceSettings && moveTimeLang && moveWifi) {
                deviceSettings.prepend(moveTimeLang, moveWifi);
              }
            }
          })
          .catch((err) => {
            console.error("Failed to fetch AP status, assuming STA mode:", err);
            isAPMode = false;
          })
          .then(() => fetch("/config.json"))
          .then((response) => response.json())
          .then((data) => {
            // everything below stays exactly as it was — just remove the old
            // "isAPMode = data.mode === 'ap'" line and the isAPMode/no-ap block
            // that used to sit here, since it's now handled above
            document.getElementById("ssid").value = data.ssid || "";
            document.getElementById("password").value = data.password || "";
            // ...rest unchanged...
            document.getElementById("ssid").value = data.ssid || "";
            document.getElementById("password").value = data.password || "";
            const apiInput = document.getElementById("openWeatherApiKey");
            if (
              data.openWeatherApiKey &&
              data.openWeatherApiKey.trim() !== ""
            ) {
              apiInput.value = MASK;
              hasSavedKey = true;
            } else {
              apiInput.value = "";
              hasSavedKey = false;
            }

            if (!window._geoApplied) {
              document.getElementById("openWeatherCity").value = data.openWeatherCity || "";
              document.getElementById("openWeatherCountry").value = data.openWeatherCountry || "";
            }
            
            document.getElementById("weatherUnits").checked =
              data.weatherUnits === "imperial";
            window._showFullTemp = !!data.showFullTemp;
            window._isImperial = data.weatherUnits === "imperial";
            document.getElementById("clockDuration").value =
              (data.clockDuration || 10000) / 1000;
            document.getElementById("weatherDuration").value =
              (data.weatherDuration || 5000) / 1000;
            document.getElementById("language").value = data.language || "";

            // --- Advanced: brightness ---
            const brightnessSlider = document.getElementById("brightnessSlider");
            const brightnessValue = document.getElementById("brightnessValue");

            brightnessSlider.value =
              typeof data.regularBrightness !== "undefined" ? data.regularBrightness : 10;

            brightnessValue.textContent =
              brightnessSlider.value == -1 ? "Off" : brightnessSlider.value;
            document.getElementById("flipDisplay").checked = !!data.flipDisplay;
            document.getElementById("ntpServer1").value = data.ntpServer1 || "";
            document.getElementById("ntpServer2").value = data.ntpServer2 || "";
            document.getElementById("twelveHourToggle").checked =
              !!data.twelveHourToggle;
            document.getElementById("showDayOfWeek").checked =
              !!data.showDayOfWeek;
            document.getElementById("showDate").checked = !!data.showDate;
            document.getElementById("showHumidity").checked =
              !!data.showHumidity;
            document.getElementById("colonBlinkEnabled").checked =
              !!data.colonBlinkEnabled;
            document.getElementById("showWeatherDescription").checked =
              !!data.showWeatherDescription;

            // --- Dimming Controls ---
            const autoDimmingEl = document.getElementById("autoDimmingEnabled");
            const dimmingEnabledEl = document.getElementById("dimmingEnabled");
            const apiInputEl = document.getElementById("openWeatherApiKey");

            const isAutoDimming =
              data.autoDimmingEnabled === true ||
              data.autoDimmingEnabled === "true" ||
              data.autoDimmingEnabled === 1;
            const isCustomDimming =
              data.dimmingEnabled === true ||
              data.dimmingEnabled === "true" ||
              data.dimmingEnabled === 1;

            autoDimmingEl.checked = isAutoDimming;
            dimmingEnabledEl.checked = isCustomDimming;

            setDimmingFieldsEnabled();

            initClockOnlyDuringDimming(data);

            // --- Donation message toggle ---
            const hideDonationEl = document.getElementById("hideDonationMsg");
            if (hideDonationEl) {
              hideDonationEl.checked = !!data.hideDonationMsg;
              hideDonationEl.addEventListener("change", function () {
                setHideDonationMsg(this.checked);
              });
            }

            if (apiInputEl)
              apiInputEl.addEventListener("input", setDimmingFieldsEnabled);
            autoDimmingEl.addEventListener("change", () => {
              if (autoDimmingEl.checked) dimmingEnabledEl.checked = false;
              setDimmingFieldsEnabled();
              clearClockOnlyIfNoDimming();
            });
            dimmingEnabledEl.addEventListener("change", () => {
              if (dimmingEnabledEl.checked) autoDimmingEl.checked = false;
              setDimmingFieldsEnabled();
              clearClockOnlyIfNoDimming();
            });

            document.getElementById("dimStartTime").value =
              (data.dimStartHour !== undefined
                ? String(data.dimStartHour).padStart(2, "0")
                : "18") +
              ":" +
              (data.dimStartMinute !== undefined
                ? String(data.dimStartMinute).padStart(2, "0")
                : "00");

            document.getElementById("dimEndTime").value =
              (data.dimEndHour !== undefined
                ? String(data.dimEndHour).padStart(2, "0")
                : "08") +
              ":" +
              (data.dimEndMinute !== undefined
                ? String(data.dimEndMinute).padStart(2, "0")
                : "00");

            document.getElementById("dimBrightness").value =
              data.dimBrightness !== undefined ? data.dimBrightness : 2;
            document.getElementById("dimmingBrightnessValue").textContent =
              document.getElementById("dimBrightness").value == -1
                ? "Off"
                : document.getElementById("dimBrightness").value;

            // --- Populate Countdown Fields ---
            document.getElementById("isDramaticCountdown").checked = !!(
              data.countdown && data.countdown.isDramaticCountdown
            );
            const countdownEnabledEl =
              document.getElementById("countdownEnabled");
            countdownEnabledEl.checked = !!(
              data.countdown && data.countdown.enabled
            );
            if (data.countdown && data.countdown.targetTimestamp) {
              const targetDate = new Date(
                data.countdown.targetTimestamp * 1000,
              );
              const year = targetDate.getFullYear();
              const month = (targetDate.getMonth() + 1)
                .toString()
                .padStart(2, "0");
              const day = targetDate.getDate().toString().padStart(2, "0");
              const hours = targetDate.getHours().toString().padStart(2, "0");
              const minutes = targetDate
                .getMinutes()
                .toString()
                .padStart(2, "0");
              document.getElementById("countdownDate").value =
                `${year}-${month}-${day}`;
              document.getElementById("countdownTime").value =
                `${hours}:${minutes}`;
            } else {
              document.getElementById("countdownDate").value = "";
              document.getElementById("countdownTime").value = "";
            }

            const countdownLabelInput =
              document.getElementById("countdownLabel");
            countdownLabelInput.addEventListener("input", function () {
              this.value = this.value
                .toUpperCase()
                .replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, "");
            });
            if (data.countdown && data.countdown.label) {
              countdownLabelInput.value = data.countdown.label
                .toUpperCase()
                .replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, "");
            } else {
              countdownLabelInput.value = "";
            }

            countdownEnabledEl.addEventListener("change", function () {
              setCountdownEnabled(this.checked);
              setCountdownFieldsEnabled(this.checked);
            });
            const dramaticCountdownEl = document.getElementById(
              "isDramaticCountdown",
            );
            dramaticCountdownEl.addEventListener("change", function () {
              setIsDramaticCountdown(this.checked);
            });
            setCountdownFieldsEnabled(countdownEnabledEl.checked);

            if (data.customMessage !== undefined) {
              document.getElementById("customMessage").value =
                data.customMessage;
            }

            if (!data.timeZone) {
              try {
                const tz = Intl.DateTimeFormat().resolvedOptions().timeZone;
                if (
                  tz &&
                  document
                    .getElementById("timeZone")
                    .querySelector(`[value="${tz}"]`)
                ) {
                  document.getElementById("timeZone").value = tz;
                } else {
                  document.getElementById("timeZone").value = "UTC";
                }
              } catch (e) {
                document.getElementById("timeZone").value = "UTC";
              }
            } else {
              document.getElementById("timeZone").value = data.timeZone;
            }
            setTimeout(async () => {
              try {
                deviceIP = await fetch("/ip").then((r) => r.text());
                document.getElementById("ipDisplay").textContent =
                  deviceIP || "—";
              } catch (e) {}

              await fetchUptimeAsync();

              if (!isAPMode) {
                try {
                  _btnData = await fetch("/get_buttons").then((r) => r.json());
                  _renderBtnConfig();
                } catch (e) {}

                try {
                  _buzzerData = await fetch("/get_buzzer").then((r) =>
                    r.json(),
                  );
                  _renderBuzzerGlobal();
                  _renderBuzzerEvents();
                  setBuzzerFieldsEnabled(_buzzerData.enabled);
                } catch (e) {}

                function getNextAlarm(alarms) {
                  const now = new Date();
                  let best = null;
                  for (const alarm of alarms) {
                    if (!alarm.enabled) continue;
                    for (let offset = 0; offset < 8; offset++) {
                      const d = new Date(now);
                      d.setDate(d.getDate() + offset);
                      if (!alarm.days[d.getDay()]) continue; // assumes days[0]=Sun..days[6]=Sat
                      d.setHours(alarm.hour, alarm.minute, 0, 0);
                      if (d <= now) continue;
                      if (!best || d < best) best = d;
                      break;
                    }
                  }
                  return best;
                }

                function formatNextAlarm(d) {
                  if (!d) return "No alarms set";
                  const now = new Date();
                  const isToday = d.toDateString() === now.toDateString();
                  const tomorrow = new Date(now);
                  tomorrow.setDate(now.getDate() + 1);
                  const isTomorrow =
                    d.toDateString() === tomorrow.toDateString();
                  const time = `${String(d.getHours()).padStart(2, "0")}:${String(d.getMinutes()).padStart(2, "0")}`;
                  const day = isToday
                    ? "Today"
                    : isTomorrow
                      ? "Tomorrow"
                      : d.toLocaleDateString(undefined, { weekday: "long" });
                  return `${day} ${time}`;
                }
                try {
                  fetch("/get_alarm")
                    .then((r) => r.json())
                    .then((d) => {
                      _alarmData = d;
                      _renderAlarmConfig();
                    })
                    .catch(() => {});
                } catch (e) {}

                const OWM_ICONS = {
                  "01d": "☀️",
                  "01n": "🌙",
                  "02d": "⛅",
                  "02n": "☁️",
                  "03d": "☁️",
                  "03n": "☁️",
                  "04d": "☁️",
                  "04n": "☁️",
                  "09d": "🌧️",
                  "09n": "🌧️",
                  "10d": "🌦️",
                  "10n": "🌧️",
                  "11d": "⛈️",
                  "11n": "⛈️",
                  "13d": "❄️",
                  "13n": "❄️",
                  "50d": "🌫️",
                  "50n": "🌫️",
                };

                function pollWeather(attempt = 0) {
                  fetch("/status?section=weather")
                    .then((r) => r.json())
                    .then((data) => {
                      const w = data.weather;
                      if (
                        w &&
                        w.currentTemperature !== undefined &&
                        w.currentTemperature !== null
                      ) {
                        document.getElementById("weather-temp").textContent = window._showFullTemp
                          ? `${w.currentTemperatureFull.toFixed(1)}°${window._isImperial ? "F" : "C"}`
                          : `${w.currentTemperature}°${window._isImperial ? "F" : "C"}`;
                        document.getElementById("weather-condition").textContent = w.descriptionShort || "";
                        document.getElementById("weather-city").textContent = w.city ? `${w.city} -` : "";
                        document.getElementById("weather-icon").textContent = OWM_ICONS[w.icon] || "";

                        document.getElementById("device-status").classList.add("loaded");
                      } else if (attempt < 15) {
                        setTimeout(() => pollWeather(attempt + 1), 2000);
                      }
                    })
.catch((e) => {
  console.error("[pollWeather] failed:", e);
  if (attempt < 15) setTimeout(() => pollWeather(attempt + 1), 2000);
});
                }
                pollWeather();
              }
            }, 100);

            document.querySelector("html").style.height = "unset";
            document.body.classList.add("loaded");
          })
          .catch((err) => {
            console.error("Failed to load config:", err);
            showSavingModal("");
            updateSavingModal(
              "<h3>⚠️ Failed to load configuration.</h3>",
              false,
            );
            removeReloadButton();
            removeRestoreButton();
            const errorMsg = (err.message || "").toLowerCase();
            if (
              errorMsg.includes("config corrupted") ||
              errorMsg.includes("failed to write config") ||
              errorMsg.includes("restore")
            ) {
              ensureRestoreButton();
            } else {
              ensureReloadButton();
            }
          });
      };

      async function submitConfig(event) {
        event.preventDefault();
        isSaving = true;

        const form = document.getElementById("configForm");
        const formData = new FormData(form);

        const clockDuration = parseInt(formData.get("clockDuration")) * 1000;
        const weatherDuration =
          parseInt(formData.get("weatherDuration")) * 1000;
        formData.set("clockDuration", clockDuration);
        formData.set("weatherDuration", weatherDuration);

        let apiKeyToSend = apiInput.value;

        // If the user left the masked key untouched, skip sending it
        if (apiKeyToSend === MASK && hasSavedKey) {
          formData.delete("openWeatherApiKey");
        } else {
          formData.set("openWeatherApiKey", apiKeyToSend);
        }

        const newHostname = (
          document.getElementById("hostnameInput").value.toLowerCase() ||
          originalHostname ||
          "esptimecast"
        ).trim();

        if (!newHostname) {
          alert("Hostname cannot be empty!");
          return;
        }

        formData.set("hostname", newHostname);

        // Advanced: ensure correct values are set for advanced fields
        formData.set(
          "brightness",
          document.getElementById("brightnessSlider").value,
        );
        formData.set(
          "flipDisplay",
          document.getElementById("flipDisplay").checked ? "on" : "",
        );
        formData.set(
          "twelveHourToggle",
          document.getElementById("twelveHourToggle").checked ? "on" : "",
        );
        formData.set(
          "showDayOfWeek",
          document.getElementById("showDayOfWeek").checked ? "on" : "",
        );
        formData.set(
          "showDate",
          document.getElementById("showDate").checked ? "on" : "",
        );
        formData.set(
          "showHumidity",
          document.getElementById("showHumidity").checked ? "on" : "",
        );
        formData.set(
          "colonBlinkEnabled",
          document.getElementById("colonBlinkEnabled").checked ? "on" : "",
        );

        // --- Dimming ---
        const autoDimmingChecked =
          document.getElementById("autoDimmingEnabled").checked;
        const customDimmingChecked =
          document.getElementById("dimmingEnabled").checked;

        // Mutual exclusivity (if both checked somehow, keep auto as priority)
        if (autoDimmingChecked && customDimmingChecked) {
          formData.set("autoDimmingEnabled", "true");
          formData.set("dimmingEnabled", "false");
        } else {
          formData.set(
            "autoDimmingEnabled",
            autoDimmingChecked ? "true" : "false",
          );
          formData.set(
            "dimmingEnabled",
            customDimmingChecked ? "true" : "false",
          );
        }

        const dimStart = document.getElementById("dimStartTime").value; // "18:45"
        const dimEnd = document.getElementById("dimEndTime").value; // "08:30"

        // Parse hour and minute
        if (dimStart) {
          const [startHour, startMin] = dimStart
            .split(":")
            .map((x) => parseInt(x, 10));
          formData.set("dimStartHour", startHour);
          formData.set("dimStartMinute", startMin);
        }
        if (dimEnd) {
          const [endHour, endMin] = dimEnd
            .split(":")
            .map((x) => parseInt(x, 10));
          formData.set("dimEndHour", endHour);
          formData.set("dimEndMinute", endMin);
        }
        formData.set(
          "dimBrightness",
          document.getElementById("dimBrightness").value,
        );
        formData.set(
          "clockOnlyDuringDimming",
          document.getElementById("clockOnlyDuringDimming").checked ? "on" : "",
        );
        formData.set(
          "showWeatherDescription",
          document.getElementById("showWeatherDescription").checked ? "on" : "",
        );
        formData.set(
          "weatherUnits",
          document.getElementById("weatherUnits").checked
            ? "imperial"
            : "metric",
        );

        // --- NEW: Countdown Form Data ---
        formData.set(
          "countdownEnabled",
          document.getElementById("countdownEnabled").checked
            ? "true"
            : "false",
        );
        formData.set(
          "isDramaticCountdown",
          document.getElementById("isDramaticCountdown").checked
            ? "true"
            : "false",
        );
        // Date and Time inputs are already handled by formData if they have a 'name' attribute
        // 'countdownDate' and 'countdownTime' are collected automatically
        // Also apply the same validation for the label when submitting
        const finalCountdownLabel = document
          .getElementById("countdownLabel")
          .value.toUpperCase()
          .replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, "");
        formData.set("countdownLabel", finalCountdownLabel);
        // --- END NEW ---

        const params = new URLSearchParams();
        for (const pair of formData.entries()) {
          params.append(pair[0], pair[1]);
        }

        // Sanitize and set customMessage before sending
        const customMsgInput = document.getElementById("customMessage");
        if (customMsgInput) {
          customMsgInput.value = customMsgInput.value
            .toUpperCase()
            .replace(safeRegex, "")
            .replace(/\s+/g, " ")
            .trim()
            .substring(0, 120);
        }

        // Check AP mode status
        let isAPMode = false;
        try {
          const apStatusResponse = await fetch("/ap_status");
          const apStatusData = await apStatusResponse.json();
          isAPMode = apStatusData.isAP;
        } catch (error) {
          console.error("Error fetching AP status:", error);
          // Handle error appropriately (e.g., assume not in AP mode)
        }

        if (isAPMode) {
          showSavingModal("");
          updateSavingModal(
            "<h3>✅ Settings saved successfully!</h3>" +
              "<p>Rebooting the device now...</p>" +
              "<p>The device will connect to your Wi-Fi.<br>" +
              "Its new IP address will appear on the display for future access.</p>",
            true, // show spinner
          );
        } else {
          showSavingModal("");
        }

        await new Promise((resolve) =>
          setTimeout(resolve, isAPMode ? 5000 : 0),
        );
        fetch("/save", {
          method: "POST",
          body: params,
        })
          .then((response) => {
            if (!response.ok) {
              return response.json().then((json) => {
                throw new Error(
                  `Server error ${response.status}: ${json.error}`,
                );
              });
            }
            return response.json();
          })
          .then((json) => {
            isSaving = false;
            removeReloadButton();
            removeRestoreButton();

            if (isAPMode) {
              setTimeout(() => {
                document.getElementById("configForm").style.display = "none";
                document.querySelector(".footer").style.display = "none";
                document.querySelector("html").style.height = "100vh";
                document.body.style.height = "100vh";
                updateSavingModal(
                  "<h3>✅ All done!</h3><p>You can now close this tab safely.</p>" +
                    "<p>Your device has rebooted and is now connected to your Wi-Fi.<br>" +
                    "Check the display for the current IP address.</p>",
                  false,
                );
              }, 5000);
              return;
            } else {
              showSavingModal("");

              const newName = document
                .getElementById("hostnameInput")
                .value.toLowerCase();
              const currentHost = window.location.hostname.replace(
                ".local",
                "",
              );
              const accessedViaIP = /^\d+\.\d+\.\d+\.\d+$/.test(
                window.location.hostname,
              );
              const hostnameChanged = newName !== originalHostname;

              if (hostnameChanged && !accessedViaIP) {
                // Hostname changed, accessed via hostname → redirect to new hostname
                let secondsLeft = 8;
                const timer = setInterval(() => {
                  secondsLeft--;
                  if (secondsLeft > 0) {
                    updateSavingModal(
                      `<h3>✅ Settings saved!</h3><p>Device is rebooting as <b>${newName}.local</b>.<br>Redirecting you in ${secondsLeft} seconds...</p>`,
                      true,
                    );
                  } else {
                    clearInterval(timer);
                    window.location.href = `http://${newName}.local`;
                  }
                }, 1000);
                updateSavingModal(
                  `<h3>✅ Settings saved!</h3><p>Device is rebooting as <b>${newName}.local</b>.<br>Redirecting you in 8 seconds...</p>`,
                  true,
                );
              } else if (hostnameChanged && accessedViaIP) {
                // Hostname changed, accessed via IP → stay on IP, inform user
                updateSavingModal(
                  `<h3>✅ Settings saved!</h3><p>Device is rebooting as <b>${newName}.local</b>.<br>You can reach it via IP or <b>${newName}.local</b> after reboot.</p>`,
                  false,
                );
                setTimeout(() => {
                  location.href = location.href.split("#")[0];
                }, 5000);
              } else {
                // Hostname unchanged → stay on current address
                updateSavingModal(
                  "<h3>✅ Configuration saved successfully.</h3><p>Device is rebooting...</p>",
                  false,
                );
                setTimeout(() => {
                  location.href = location.href.split("#")[0];
                }, 3000);
              }
            }
          })
          .catch((err) => {
            isSaving = false;

            if (isAPMode && err.message.includes("Failed to fetch")) {
              console.warn("Expected disconnect in AP mode after reboot.");
              showSavingModal("");
              updateSavingModal(
                "<h3>✅ Settings saved successfully!</h3><p>Rebooting the device now...</p>",
                false,
              );
              setTimeout(() => {
                document.getElementById("configForm").style.display = "none";
                updateSavingModal(
                  "<h3>✅ All done!</h3><p>You can now close this tab safely.</p>" +
                    "<p>Your device has rebooted and is now connected to your Wi-Fi.<br>" +
                    "Check the display for the current IP address.</p>",
                  false,
                );
              }, 5000);
              removeReloadButton();
              removeRestoreButton();
              return;
            }

            console.error("Save error:", err);
            let friendlyMessage =
              "<h3>⚠️ Something went wrong while saving the configuration.</h3>";
            if (err.message.includes("Failed to fetch")) {
              friendlyMessage =
                "<h3>⚠️ Cannot connect to the device.</h3><p>Is it powered on and connected?</p>";
            }

            updateSavingModal(
              `${friendlyMessage}<p>Details: ${err.message}</p>`,
              false,
            );

            // Show only one action button, based on error content
            removeReloadButton();
            removeRestoreButton();
            const errorMsg = (err.message || "").toLowerCase();
            if (
              errorMsg.includes("config corrupted") ||
              errorMsg.includes("failed to write config") ||
              errorMsg.includes("restore")
            ) {
              ensureRestoreButton();
            } else {
              ensureReloadButton();
            }
          });
      }

      function showSavingModal(message) {
        let modal = document.getElementById("savingModal");
        if (!modal) {
          modal = document.createElement("div");
          modal.id = "savingModal";
          modal.innerHTML = `
      <div id="savingModalContent">
        <div class="spinner"></div>
        <div id="savingModalText">${message}</div>
      </div>
    `;
          document.body.appendChild(modal);
        } else {
          document.getElementById("savingModalText").innerHTML = message;
          document.querySelector("#savingModal .spinner").style.display =
            "block";
        }
        modal.style.display = "flex";
        document.body.classList.add("modal-open");
      }

      function updateSavingModal(message, showSpinner = false) {
        let modalText = document.getElementById("savingModalText");
        modalText.innerHTML = message;
        document.querySelector("#savingModal .spinner").style.display =
          showSpinner ? "block" : "none";

        // Remove reload/restore buttons if no longer needed
        if (
          message.includes("saved successfully") ||
          message.includes("Backup restored") ||
          message.includes("All done!")
        ) {
          removeReloadButton();
          removeRestoreButton();
        }
      }

      function ensureReloadButton(options = {}) {
        let modalContent = document.getElementById("savingModalContent");
        if (!modalContent) return;
        let btn = document.getElementById("reloadButton");
        if (!btn) {
          btn = document.createElement("button");
          btn.id = "reloadButton";
          btn.className = "primary-button";
          btn.style.display = "inline-block";
          btn.style.margin = "1rem 0 0 0";
          modalContent.appendChild(btn);
        }
        btn.textContent = options.text || "Reload Page";
        btn.onclick = options.onClick || (() => location.reload());
        btn.style.display = "inline-block";
        return btn;
      }

      function ensureRestoreButton(options = {}) {
        let modalContent = document.getElementById("savingModalContent");
        if (!modalContent) return;
        let btn = document.getElementById("restoreButton");
        if (!btn) {
          btn = document.createElement("button");
          btn.id = "restoreButton";
          btn.className = "primary-button";
          btn.style.display = "inline-block";
          btn.style.margin = "1rem 0 0 0.5rem";
          modalContent.appendChild(btn);
        }
        btn.textContent = options.text || "Restore Backup";
        btn.onclick = options.onClick || restoreBackupConfig;
        btn.style.display = "inline-block";
        return btn;
      }

      function removeReloadButton() {
        let btn = document.getElementById("reloadButton");
        if (btn && btn.parentNode) btn.parentNode.removeChild(btn);
      }
      function removeRestoreButton() {
        let btn = document.getElementById("restoreButton");
        if (btn && btn.parentNode) btn.parentNode.removeChild(btn);
      }
      function restoreBackupConfig() {
        showSavingModal("<h3>Restoring backup...</h3>");
        removeReloadButton();
        removeRestoreButton();

        fetch("/restore", { method: "POST" })
          .then((response) => {
            if (!response.ok) {
              throw new Error("Server returned an error");
            }
            return response.json();
          })
          .then((data) => {
            updateSavingModal(
              "<h3>✅ Backup restored!</h3><p>Device will now reboot.</p>",
            );
            setTimeout(() => location.reload(), 1500);
          })
          .catch((err) => {
            console.error("Restore error:", err);
            updateSavingModal(
              `<h3>❌ Failed to restore backup:</h3><p>${err.message}</p>`,
              false,
            );

            // Show only one button, for backup restore failures show reload.
            removeReloadButton();
            removeRestoreButton();
            ensureReloadButton();
          });
      }

      function hideSavingModal() {
        const modal = document.getElementById("savingModal");
        if (modal) {
          modal.style.display = "none";
          document.body.classList.remove("modal-open");
        }
      }

      // We add 'index' here to track which collapsible we are on
      // 1. Target only the inner categories (Time, Weather, etc.)
      document.querySelectorAll(".sub-collapsible").forEach((button) => {
        const content = button.nextElementSibling;

        // Existing click logic (keep this exactly as is)
        button.addEventListener("click", () => {
          const isOpen = button.classList.toggle("open");
          button.setAttribute("aria-expanded", isOpen);
          content.setAttribute("aria-hidden", !isOpen);

          if (isOpen) {
            content.style.height = content.scrollHeight + "px";
            content.addEventListener("transitionend", function handler() {
              content.style.height = "auto";
              content.removeEventListener("transitionend", handler);
            });
          } else {
            content.style.height = content.scrollHeight + "px";
            void content.offsetHeight;
            content.style.height = "0px";
          }
        });

        // SURGICAL CHANGE: Force all INNER menus to be open on load
        button.classList.add("open");
        button.setAttribute("aria-expanded", "true");
        content.setAttribute("aria-hidden", "false");
        content.style.height = "auto";
      });

      let brightnessDebounceTimeout = null;

      function setBrightnessLive(val) {
        // Cancel the previous timeout if it exists
        if (brightnessDebounceTimeout) {
          clearTimeout(brightnessDebounceTimeout);
        }

        // Set a new timeout
        brightnessDebounceTimeout = setTimeout(() => {
          fetch("/set_brightness", {
            method: "POST",
            headers: {
              "Content-Type": "application/x-www-form-urlencoded",
              "X-Source": "UI",
            },
            body: "value=" + encodeURIComponent(val),
          })
            .then((res) => res.json())
            .catch((e) => {}); // Optionally handle errors
        }, 150); // 150ms debounce
      }

      function setFlipDisplay(val) {
        fetch("/set_flip", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        });
      }

      function setTwelveHour(val) {
        fetch("/set_twelvehour", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        });
      }

      function setShowDayOfWeek(val) {
        fetch("/set_dayofweek", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        });
      }

      function setShowDate(val) {
        fetch("/set_showdate", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        });
      }

      function setColonBlink(val) {
        fetch("/set_colon_blink", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        });
      }

      function setShowHumidity(val) {
        fetch("/set_humidity", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        });
      }

      function setLanguage(val) {
        fetch("/set_language", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + encodeURIComponent(val),
        });
      }

      function setShowWeatherDescription(val) {
        fetch("/set_weatherdesc", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        });
      }

      function setWeatherUnits(val) {
        window._isImperial = !!val;
        fetch("/set_units", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        })
          .then((res) => {
            if (!res.ok) throw new Error("Failed to set units");
            refetchWeatherAfterUnitChange();
          })
          .catch(() => {});
      }

      function refetchWeatherAfterUnitChange() {
        setTimeout(() => {
          fetch("/status?section=weather")
            .then((r) => r.json())
            .then((data) => {
              const w = data.weather;
              if (w) {
                document.getElementById("weather-temp").textContent = window._showFullTemp
                  ? `${w.currentTemperatureFull.toFixed(1)}°${window._isImperial ? "F" : "C"}`
                  : `${w.currentTemperature}°${window._isImperial ? "F" : "C"}`;
                document.getElementById("weather-condition").textContent =
                  w.descriptionShort || "";
                document.getElementById("weather-city").textContent = w.city
                  ? `${w.city} -`
                  : "";
                document.getElementById("weather-icon").textContent =
                  OWM_ICONS[w.icon] || "";
              }
            })
            .catch(() => {});
        }, 1000);
      }

      // --- Clock-only-during-dimming setter (no reboot) ---
      function setClockOnlyDuringDimming(val) {
        fetch("/set_clock_only_dimming", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        }).catch((e) => {
          console.error("Failed to set clockOnlyDuringDimming:", e);
        });
      }

      // --- Donation message opt-out toggle (no reboot) ---
      function setHideDonationMsg(val) {
        fetch("/set_hide_donation", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0),
        }).catch((e) => {
          console.error("Failed to set hideDonationMsg:", e);
        });
      }

      // Initialize the checkbox from cfg and wire up immediate save (no reboot)
      function initClockOnlyDuringDimming(cfg) {
        const el = document.getElementById("clockOnlyDuringDimming");
        if (!el) return;

        // Set initial checked value from config
        el.checked = !!cfg.clockOnlyDuringDimming;

        // Determine whether dimming is enabled (auto OR custom)
        const autoDim =
          cfg.autoDimmingEnabled === true ||
          cfg.autoDimmingEnabled === "true" ||
          cfg.autoDimmingEnabled === 1;
        const manualDim =
          cfg.dimmingEnabled === true ||
          cfg.dimmingEnabled === "true" ||
          cfg.dimmingEnabled === 1;

        // Enable whenever some dimming mode is active — regardless of dimBrightness,
        // since interrupts (alarms/timers) can still light the display even at dimBrightness -1
        el.disabled = !(autoDim || manualDim);

        // On change, persist immediately (no reboot)
        el.addEventListener("change", function () {
          const want = this.checked;
          setClockOnlyDuringDimming(want);
          setAlarmFieldsEnabled(
            document.getElementById("alarmEnabled").checked,
          );
          // optimistic UI: leave checkbox as toggled; if server fails we don't roll back here
        });
      }

      // --- Countdown Controls Logic ---
      // NEW: Function to enable/disable countdown specific fields
      function setCountdownFieldsEnabled(enabled) {
        document.getElementById("countdownLabel").disabled = !enabled;
        document.getElementById("countdownDate").disabled = !enabled;
        document.getElementById("countdownTime").disabled = !enabled;
        document.getElementById("isDramaticCountdown").disabled = !enabled;
      }

      // Existing function to send countdown enable/disable command to ESP
      function setCountdownEnabled(val) {
        fetch("/set_countdown_enabled", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0), // Send 1 for true, 0 for false
        });
      }

      function setIsDramaticCountdown(val) {
        fetch("/set_dramatic_countdown", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: "value=" + (val ? 1 : 0), // Send 1 for true, 0 for false
        });
      }
      // --- END Countdown Controls Logic ---

function getLocation() {
  const returnUrl = window.location.href.split("?")[0]; // strip any existing geo params
  window.location.href =
    "https://esptimecast.com/geo.html?return=" + encodeURIComponent(returnUrl);
}

// Run this once on page load to pick up results after the redirect back.
function handleGeoRedirectResult() {
  const params = new URLSearchParams(window.location.search);
  const btn = document.getElementById("geo-button");
  let hadGeoParams = true;

  if (params.has("geoLat") && params.has("geoLon")) {
    const lat = params.get("geoLat");
    const lon = params.get("geoLon");
    const city = params.get("geoCity") || "";
    const country = params.get("geoCountry") || "";

    document.getElementById("openWeatherCity").value = lat;
    document.getElementById("openWeatherCountry").value = lon;
    window._geoApplied = true; 

    btn.textContent = "Location: " + (city || country || "Found");
    btn.disabled = true;
    btn.classList.add("geo-disabled");

    setTimeout(saveWeatherConfig, 1000);
  } else if (params.has("geoError")) {
    const errorMessages = {
      denied: "Location permission denied.",
      timeout: "Location request timed out.",
      unsupported: "Location is not supported by this browser.",
      unknown: "Unable to detect location.",
    };
    // Scroll first so the alert appears with the right section already in view.
    document.getElementById("weather-settings")?.scrollIntoView({ behavior: "smooth", block: "start" });
    alert(
      (errorMessages[params.get("geoError")] || "Unable to detect location.") +
        "\n\nYou can manually enter your coordinates instead."
    );
  } else {
    hadGeoParams = false;
  }

function scrollToWeatherSettings() {
  document.getElementById("weather-settings")?.scrollIntoView({ behavior: "smooth", block: "start" });
}

  if (hadGeoParams) {
  if (document.body.classList.contains("loaded")) {
    // Already loaded (e.g. fade-in was instant or finished before this ran)
    scrollToWeatherSettings();
  } else {
    // Wait for the fade-in's opacity transition to actually finish before
    // scrolling, so nothing in the load sequence overwrites our scroll.
    const onTransitionEnd = (e) => {
      if (e.propertyName === "opacity") {
        document.body.removeEventListener("transitionend", onTransitionEnd);
        scrollToWeatherSettings();
      }
    };
    document.body.addEventListener("transitionend", onTransitionEnd);

    // Fallback in case transitionend never fires (e.g. duration is 0,
    // or the .loaded class add happens on a different element).
    setTimeout(scrollToWeatherSettings, 1000);
  }
    const url = new URL(window.location.href);
    ["geoLat", "geoLon", "geoAccuracy", "geoCity", "geoCountry", "geoCountryCode", "geoError"].forEach(
      (key) => url.searchParams.delete(key)
    );
    window.history.replaceState({}, "", url.toString());
  }
}

window.addEventListener("load", handleGeoRedirectResult);

      // --- OpenWeather API Key field UX ---
      const MASK_LENGTH = 32;
      const MASK = "*".repeat(MASK_LENGTH);
      const apiInput = document.getElementById("openWeatherApiKey");
      let hasSavedKey = false;

      // --- Initialize the field after config load ---
      if (apiInput.value && apiInput.value.trim() !== "") {
        apiInput.value = MASK; // show mask
        hasSavedKey = true;
      } else {
        apiInput.value = "";
        hasSavedKey = false;
      }

      // --- Detect user clearing intent ---
      apiInput.addEventListener("input", () => {
        apiInput.dataset.clearing = apiInput.value === "" ? "true" : "false";
      });

      // --- Handle Delete/Backspace when focused but empty ---
      apiInput.addEventListener("keydown", (e) => {
        if (
          (e.key === "Backspace" || e.key === "Delete") &&
          apiInput.value === ""
        ) {
          apiInput.dataset.clearing = "true";
        }
      });

      // --- Focus handler: clear mask for editing ---
      apiInput.addEventListener("focus", () => {
        if (apiInput.value === MASK) apiInput.value = "";
      });

      // --- Blur handler: restore mask if user didn’t clear the field ---
      apiInput.addEventListener("blur", () => {
        if (apiInput.value === "") {
          if (hasSavedKey && apiInput.dataset.clearing !== "true") {
            apiInput.value = MASK; // remask
          } else {
            hasSavedKey = false; // user cleared the key
            apiInput.dataset.clearing = "false";
            apiInput.value = ""; // leave blank
            setDimmingFieldsEnabled();
          }
        }
      });

      // --- Uptime Tracker ---
      let sessionSeconds = 0;
      let totalSeconds = 0;
      let uptimeTimer;

      function fetchUptime() {
        return fetch("/uptime")
          .then((res) => res.json())
          .then((data) => {
            const hostInput = document.getElementById("hostnameInput");
            if (hostInput) {
              hostInput.value = data.hostname || "esptimecast";
              resizeHostname(hostInput);
              originalHostname = data.hostname || "esptimecast";
            }
            // Use the NEW keys we created in the firmware
            sessionSeconds = data.session_seconds || 0;
            totalSeconds = data.total_seconds || 0;

            // Update firmware version
            const versionEl = document.getElementById("fwVersion");
            if (versionEl) versionEl.textContent = "v" + data.version;

            // Update displays immediately
            updateUptimeDisplay();

            // Clear existing timer and start fresh
            if (uptimeTimer) clearInterval(uptimeTimer);
            uptimeTimer = setInterval(() => {
              sessionSeconds++;
              totalSeconds++;
              updateUptimeDisplay();
            }, 1000);
          })
          .catch((err) => console.error("Error fetching /uptime:", err));
      }

      function fetchUptimeAsync() {
        return fetch("/uptime")
          .then((res) => res.json())
          .then((data) => {
            const hostInput = document.getElementById("hostnameInput");
            if (hostInput) {
              hostInput.value = data.hostname || "esptimecast";
              resizeHostname(hostInput);
              originalHostname = data.hostname || "esptimecast";
            }
            sessionSeconds = data.session_seconds || 0;
            totalSeconds = data.total_seconds || 0;

            const versionEl = document.getElementById("fwVersion");
            if (versionEl) versionEl.textContent = "v" + data.version;

            updateUptimeDisplay();

            if (uptimeTimer) clearInterval(uptimeTimer);
            uptimeTimer = setInterval(() => {
              sessionSeconds++;
              totalSeconds++;
              updateUptimeDisplay();
            }, 1000);
          })
          .catch((err) => console.error("Error fetching /uptime:", err));
      }

      function updateUptimeDisplay() {
        const sessionEl = document.getElementById("sessionDisplay");
        const totalEl = document.getElementById("totalDisplay");

        if (sessionEl) sessionEl.textContent = formatUptime(sessionSeconds);
        if (totalEl) totalEl.textContent = formatUptime(totalSeconds);
      }

      // Keep your existing formatUptime function as is
      function formatUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        seconds %= 86400;
        const h = Math.floor(seconds / 3600);
        const m = Math.floor((seconds % 3600) / 60);
        const s = seconds % 60;
        const timePart = `${String(h).padStart(2, "0")}:${String(m).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
        if (days > 1) return `${days} days ${timePart}`;
        if (days === 1) return `1 day ${timePart}`;
        return timePart;
      }

      function sendCustomMessage() {
        const input = document.getElementById("customMessage");
        let rawValue = input.value.toUpperCase();

        // Detect digits in brackets and set flag
        let useBigNumbers = "0";
        if (/\[\d+\]/.test(rawValue)) {
          useBigNumbers = "1";
        }

        // Clean message
        let message = rawValue
          .replace(safeRegex, "")
          .replace(/\s+/g, " ")
          .trim()
          .substring(0, 120);

        if (message.length === 0 && input.value.trim().length > 0) return;

        // All data goes into the URL for a GET request
        const url = `/action?message=${encodeURIComponent(message)}&bignumbers=${useBigNumbers}&scrolls=0&seconds=0`;

        fetch(url, {
          method: "GET",
          headers: {
            "X-Source": "UI",
          },
        })
          .then((res) => {
            if (res.status === 409) {
              throw new Error(
                "Display temporarily locked. " +
                  "A protected message is active or the clock is in dimming mode.",
              );
            }

            if (!res.ok) throw new Error("Failed to send message.");

            return res.text();
          })
          .then((res) => {
            showToast("✓ Message sent", 3000);
          })
          .catch((err) => {
            console.error("Error sending custom message:", err);

            if (err.message.includes("protected")) {
              showToast("⚠️ \u00A0\u00A0Display temporarily locked", 5000);
            } else {
              showToast("⚠️ \u00A0\u00A0Failed to send message", 3000);
            }
          });
      }

      function clearCustomMessage() {
        // Use /action?message= to clear the message
        const url = "/action?message=&scrolls=0&seconds=0";

        fetch(url, {
          method: "GET",
          headers: {
            "X-Source": "UI",
          },
        })
          .then((res) => {
            if (!res.ok) throw new Error("Failed to clear message.");

            return res.text();
          })
          .then((res) => {
            document.getElementById("customMessage").value = "";

            showToast("✓ Custom message cleared", 3000);
          })
          .catch((err) => {
            console.error("Error clearing custom message:", err);

            showToast("⚠️ \u00A0\u00A0Failed to clear message", 3000);
          });
      }

      // --- Dimming Controls Logic (The correct version) ---
      function setDimmingFieldsEnabled() {
        const apiKeyField = document.getElementById("openWeatherApiKey");
        const autoDimming = document.getElementById("autoDimmingEnabled");
        const dimmingEnabled = document.getElementById("dimmingEnabled");
        const dimStart = document.getElementById("dimStartTime");
        const dimEnd = document.getElementById("dimEndTime");
        const dimBrightness = document.getElementById("dimBrightness");
        const noteEl = document.getElementById("autoDimmingNote");

        if (!apiKeyField || !autoDimming || !dimmingEnabled) return;

        const currentApiKeyInput = apiKeyField.value.trim();
        // Checks if a key is saved (hasSavedKey) OR if the user is currently typing a new one.
        const isKeyPresent =
          hasSavedKey ||
          (currentApiKeyInput !== "" && currentApiKeyInput !== MASK);

        // --- 1. Control Auto Dimming based on Key Presence ---
        // Meets requirement: "when page load after autodim has been saved to json,
        // if user removes the api key (masked) the toggle auto dim toggle should get disabled"
        if (!isKeyPresent) {
          autoDimming.checked = false;
          autoDimming.disabled = true;
          if (noteEl) noteEl.style.display = "block";
        } else {
          autoDimming.disabled = false;
          if (noteEl) noteEl.style.display = "none";
        }

        // Custom Dimming toggle is always enabled (since it's not key-dependent)
        dimmingEnabled.disabled = false;

        // --- 2. Control Dependent Fields based on Active Mode ---

        const isAutoDimmingActive = autoDimming.checked && isKeyPresent; // Auto is only active if checked AND key is present
        const isCustomDimmingActive = dimmingEnabled.checked;
        const isDimmingActive = isAutoDimmingActive || isCustomDimmingActive; // Brightness slider logic

        // --- Update Clock-only-during-dimming checkbox state (if present) ---
        const clockOnlyEl = document.getElementById("clockOnlyDuringDimming");
        if (clockOnlyEl) {
          const currentlyDimEnabled =
            isAutoDimmingActive || isCustomDimmingActive;
          clockOnlyEl.disabled = !currentlyDimEnabled;
        }

        // BRIGHTNESS SLIDER: Enabled if EITHER mode is active.
        if (dimBrightness) {
          dimBrightness.disabled = !isDimmingActive;
        }

        // START/END TIME FIELDS: Enabled ONLY if Custom Dimming is checked (key not needed).
        const isCustomTimeEnabled = dimmingEnabled.checked;
        if (dimStart) {
          dimStart.disabled = !isCustomTimeEnabled;
        }
        if (dimEnd) {
          dimEnd.disabled = !isCustomTimeEnabled;
        }

        clearClockOnlyIfNoDimming();
      }

      // If both dimming modes are disabled, clear & persist the Clock-only-during-dimming flag
      function clearClockOnlyIfNoDimming() {
        const autoEl = document.getElementById("autoDimmingEnabled");
        const dimEl = document.getElementById("dimmingEnabled");
        const clockEl = document.getElementById("clockOnlyDuringDimming");
        if (!autoEl || !dimEl || !clockEl) return;

        if (!autoEl.checked && !dimEl.checked) {
          // if currently checked, uncheck and persist change immediately
          if (clockEl.checked) {
            clockEl.checked = false;
            // persist without reboot
            fetch("/set_clock_only_dimming", {
              method: "POST",
              body: new URLSearchParams({ value: "false" }),
            }).catch((e) =>
              console.error(
                "Failed to persist clockOnlyDuringDimming clear:",
                e,
              ),
            );
          }
          // also ensure it's disabled in the UI
          clockEl.disabled = true;
        }
      }

      window.addEventListener("DOMContentLoaded", () => {
        const apiKeyEl = document.getElementById("openWeatherApiKey");
        const autoEl = document.getElementById("autoDimmingEnabled");
        const dimEl = document.getElementById("dimmingEnabled");

        if (apiKeyEl) {
          apiKeyEl.addEventListener("input", setDimmingFieldsEnabled);
          apiKeyEl.addEventListener("change", setDimmingFieldsEnabled);
        }
        if (autoEl) autoEl.addEventListener("change", setDimmingFieldsEnabled);
        if (dimEl) dimEl.addEventListener("change", setDimmingFieldsEnabled);

        const dimBrightnessEl = document.getElementById("dimBrightness");
        if (dimBrightnessEl) {
          dimBrightnessEl.addEventListener("input", setDimmingFieldsEnabled);
        }
      });

      const ssidInput = document.getElementById("ssid");
      const list = document.getElementById("ssidList");
      const scanBtn = document.getElementById("scanBtn");
      const arrowBtn = document.getElementById("arrowBtn");

      // Unlock the arrow button UI
      function enableDropdown() {
        arrowBtn.disabled = false;
        arrowBtn.style.opacity = "1";
        arrowBtn.style.cursor = "pointer";
      }

      // Show/Hide the dropdown list
      function toggleList(e) {
        if (e) e.stopPropagation();
        if (list.children.length > 0) {
          list.style.display =
            list.style.display === "block" ? "none" : "block";
        }
      }

      arrowBtn.onclick = toggleList;

      // Close dropdown if user clicks away
      window.onclick = (e) => {
        if (!e.target.matches("#arrowBtn") && !e.target.matches("#ssid")) {
          list.style.display = "none";
        }
      };

      scanBtn.onclick = async function () {
        // 1. Prepare UI
        arrowBtn.disabled = true;
        scanBtn.disabled = true;
        list.style.display = "none";

        // 2. Start Continuous Dot Animation
        let dotCount = 0;
        const dotInterval = setInterval(() => {
          dotCount = (dotCount % 3) + 1;
          scanBtn.innerText = ".".repeat(dotCount);
        }, 850);

        // 3. Define the recursive Polling Function
        const performPolling = async () => {
          try {
            const resp = await fetch("/scan");

            if (resp.status === 202) {
              // ESP is still busy. Wait 1 second then try again.
              await new Promise((resolve) => setTimeout(resolve, 1000));
              return await performPolling();
            }

            if (resp.status === 200) {
              const networks = await resp.json();
              list.innerHTML = "";

              if (networks && networks.length > 0) {
                networks.forEach((net) => {
                  const div = document.createElement("div");
                  div.className = "ssid-option";
                  div.innerText = net.ssid;
                  div.onclick = () => {
                    ssidInput.value = net.ssid;
                    list.style.display = "none";
                  };
                  list.appendChild(div);
                });
                enableDropdown();
                list.style.display = "block";
              } else {
                alert("No networks found.");
              }
            }
          } catch (err) {
            console.error("Scan error:", err);
            alert("Device connection lost.");
          }
        };

        // 4. Run the polling chain
        await performPolling();

        // 5. Final Cleanup (Runs only AFTER polling is completely finished)
        clearInterval(dotInterval);
        scanBtn.disabled = false;
        scanBtn.innerText = "Scan";
      };

      async function checkUpdate() {
        const checkBtn = document.getElementById("btn-check-ota");
        const updateDiv = document.getElementById("ota-update-found");
        const statusText = document.getElementById("ota-status-text");

        checkBtn.disabled = true;
        updateDiv.style.display = "none";
        statusText.style.color = "";
        statusText.style.fontWeight = "normal";

        try {
          // STEP 1: Get current version and specific board type
          const localRes = await fetch("/get_version");
          const localData = await localRes.json();
          const currentVersion = localData.version;
          const board = localData.board; // e.g., "esp32s3"

          // STEP 2: Fetch latest info from update server
          const updateRes = await fetch(
            "https://esptimecast.com/firmware.json?t=" + Date.now(),
          );
          if (!updateRes.ok)
            throw new Error(`Update server returned ${updateRes.status}`);

          const updateData = await updateRes.json();
          const latestVersion = updateData.version;

          // STEP 3: Semantic Comparison
          const parseV = (v) =>
            v
              .replace(/[^\d.]/g, "")
              .split(".")
              .map(Number);
          const vRemote = parseV(latestVersion);
          const vLocal = parseV(currentVersion);

          let isNewer = false;
          for (let i = 0; i < 3; i++) {
            if ((vRemote[i] || 0) > (vLocal[i] || 0)) {
              isNewer = true;
              break;
            }
            if ((vRemote[i] || 0) < (vLocal[i] || 0)) {
              isNewer = false;
              break;
            }
          }

          if (isNewer) {
            // DYNAMIC LINK SELECTION:
            // Matches "esp32s3" from device to "esp32s3" in ota.json
            pendingBinUrl = updateData.bins[board];
            pendingFromVersion = currentVersion;
            if (!pendingBinUrl) {
              throw new Error(`No binary found for board: ${board}`);
            }

            console.log(`Found Update for ${board}:`, pendingBinUrl);

            statusText.style.fontWeight = "bold";
            statusText.style.color = "#2ecc71";
            statusText.innerText = `New v${latestVersion} found! (Current: v${currentVersion})`;

            checkBtn.style.display = "none";
            updateDiv.style.display = "block";
          } else {
            showToast(`Up to date (v${currentVersion})`, 3000);
            setTimeout(() => {
              checkBtn.disabled = false;
              statusText.innerText = "";
            }, 3000);
          }
        } catch (e) {
          console.error("OTA Check Error:", e);
          statusText.style.color = "#ff4444";
          showToast(`⚠️ \u00A0\u00A0${e.message}`, 5000);
          setTimeout(() => {
            checkBtn.disabled = false;
          }, 5000);
        }
      }

      async function performUpdate() {
        if (!pendingBinUrl) {
          alert("No update URL found. Please check for updates again.");
          return;
        }

        // 1. Initial UI feedback
        showSavingModal(
          "<h3>🚀 Preparing Device...</h3><p>Entering update mode.</p>",
        );

        try {
          // STEP 1: Signal the ESP to enter "Soft Maintenance" mode
          // This clears the screen and stops background tasks on the ESP
          const prepRes = await fetch(`/perform_update`);
          if (!prepRes.ok)
            throw new Error("Device refused to enter update mode.");

          // Brief pause to let the ESP UI update (show icon)
          await new Promise((r) => setTimeout(r, 800));

          // STEP 2: Browser downloads the .bin from GitHub
          updateSavingModal(
            "<h3>Step 1/2: Downloading...</h3><p>Fetching firmware from GitHub.</p>",
            true,
          );

          const fileRes = await fetch(
            pendingBinUrl + "?from=" + encodeURIComponent(pendingFromVersion),
          );
          if (!fileRes.ok)
            throw new Error("Could not download firmware from GitHub.");

          const blob = await fileRes.blob();
          console.log("Downloaded blob size:", blob.size);

          // STEP 3: Browser uploads the blob to the ESP via POST
          updateSavingModal(
            "<h3>Step 2/2: Uploading...</h3>" +
              "<p>Writing to flash memory.</p>" +
              "<span id='ota-progress-bar' class='ota-progress-value'>0%</span>",
            true,
          );

          const formData = new FormData();
          formData.append("update", blob, "update.bin");

          const xhr = new XMLHttpRequest();
          xhr.open("POST", "/upload_ota");

          // Track upload progress
          xhr.upload.onprogress = (e) => {
            if (e.lengthComputable) {
              const percent = Math.round((e.loaded / e.total) * 100);
              const progressSpan = document.getElementById("ota-progress-bar");
              if (progressSpan) progressSpan.innerText = percent + "%";
            }
          };

          //Handle completion
          xhr.onload = function () {
            if (xhr.status === 200 && xhr.responseText.includes("OK")) {
              document.getElementById("configForm").style.display = "none";
              document.querySelector(".footer").style.display = "none";
              document.querySelector("html").style.height = "100vh";
              document.body.style.height = "100vh";
              updateSavingModal(
                "<h3>✅ Update Successful!</h3>" +
                  "<p><b>ESPTimeCast</b> has been updated to the latest version.<br>Thank you for being part of the community.</p>" +
                  "<p>❤️ <b>Built with love in Osaka!</b></p>" +
                  "<p><span id='modal-countdown'>Device is rebooting in 25</span></p>",
                false,
              );
              // Start countdown for page refresh
              let count = 25;
              const timer = setInterval(() => {
                count--;
                const counter = document.getElementById("modal-countdown");
                if (counter)
                  counter.innerText = `Device is rebooting in ${count}s`;
                if (count <= 0) {
                  clearInterval(timer);
                  location.reload();
                }
              }, 1000);
            } else {
              updateSavingModal(
                "<h3>❌ Upload Failed</h3><p>The device rejected the file.</p>",
                false,
              );
            }
          };

          xhr.onerror = () => {
            updateSavingModal(
              "<h3>❌Connection Lost</h3><p>Check your Wi-Fi and try again.</p>",
              false,
            );
          };

          xhr.send(formData);
        } catch (e) {
          console.error("OTA Error:", e);
          updateSavingModal(
            "<h3>❌ Update Error</h3><p>" + e.message + "</p>",
            false,
          );
        }
      }

      document.addEventListener("keydown", function (event) {
        if (event.key === "Enter") {
          const activeElem = document.activeElement;

          // SCENARIO 1: User is typing a Custom Message
          if (activeElem.id === "customMessage") {
            event.preventDefault();
            sendCustomMessage();
          }
          // SCENARIO 2: User is in any other input field (NTP, Brightness, etc.)
          else if (
            activeElem.tagName === "INPUT" ||
            activeElem.tagName === "SELECT"
          ) {
            event.preventDefault();
            // Manually trigger the form submit or click the Save button
            document.querySelector('input[type="submit"]').click();
          }
        }
      });

      function resizeHostname(el) {
        el.style.width = `calc(1rem + ${el.value.length}ch)`;
      }

      // ---------------------------------------------------------------
      // Physical Buttons Configuration
      // ---------------------------------------------------------------
      const BTN_ACTIONS = [
        ["", "- Disabled -"],
        ["next_mode", "Next Mode"],
        ["prev_mode", "Previous Mode"],
        ["brightness_up", "Brightness +"],
        ["brightness_down", "Brightness −"],
        ["display_off", "Display Off"],
        ["display_on", "Display On"],
        ["flip", "Flip Display"],
        ["twelvehour", "Toggle 12h Clock"],
        ["dayofweek", "Toggle Day of Week"],
        ["showdate", "Toggle Show Date"],
        ["colon_blink", "Toggle Colon Blink"],
        ["humidity", "Toggle Humidity"],
        ["weatherdesc", "Toggle Weather Description"],
        ["countdown_enabled", "Toggle Countdown"],
        ["enable_rotation", "Toggle Rotation"],
        ["alarm_stop", "Stop Alarm"],
        ["alarm_snooze", "Snooze Alarm"],
        ["timer_pause", "Pause Timer"],
        ["timer_resume", "Resume Timer"],
        ["timer_stop", "Stop Timer"],
        ["stopwatch", "Start Stopwatch"],
        ["stopwatch_resume", "Resume Stopwatch"],
        ["stopwatch_stop", "Pause Stopwatch"],
        ["stopwatch_restart", "Restart Stopwatch"],
        ["stopwatch_reset", "Reset Stopwatch"],
        ["stopwatch_clear", "Exit Stopwatch"],
        ["pomodoro_start", "Start Pomodoro"],
        ["pomodoro_stop", "Stop Pomodoro"],
        ["pomodoro_pause", "Pause Pomodoro"],
        ["pomodoro_resume", "Resume Pomodoro"],
        ["clear_message", "Clear Temporary Message"],
        ["clear_message_all", "Clear All Messages"],
        ["restart", "Restart Device"],
      ];

      // Common safe-to-use GPIO (excludes 6-11 flash pins on most ESP32 modules)
      const ALL_GPIO = [
        0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26,
        27, 32, 33, 34, 35, 36, 39,
      ];

      let _btnData = null;

      function _btnActionOpts(sel) {
        return BTN_ACTIONS.map(
          ([v, l]) =>
            `<option value="${v}"${v === sel ? " selected" : ""}>${l}</option>`,
        ).join("");
      }

      function _syncBtnData() {
        if (!_btnData) return;

        for (let i = 0; i < 4; i++) {
          const p = document.getElementById(`btn${i + 1}_pin`);
          const s = document.getElementById(`btn${i + 1}_short`);
          const l = document.getElementById(`btn${i + 1}_long`);

          if (p) _btnData.buttons[i].pin = parseInt(p.value);
          if (s) _btnData.buttons[i].shortAction = s.value;
          if (l) _btnData.buttons[i].longAction = l.value;
        }
      }

      function _btnPinOpts(selPin, excludePins) {
        const ex = new Set(excludePins.map(Number));
        let out = `<option value="-1"${selPin === -1 ? " selected" : ""}>- Disabled -</option>`;
        ALL_GPIO.forEach((p) => {
          if (!ex.has(p))
            out += `<option value="${p}"${selPin === p ? " selected" : ""}>GPIO ${p}</option>`;
        });
        return out;
      }

      let _btnExpanded = [true, false, false, false]; // btn1 always open

      function _renderBtnConfig() {
        const el = document.getElementById("btn-config-container");
        if (!el || !_btnData) return;

        // Auto-expand if a pin is already configured
        _btnData.buttons.forEach((b, i) => {
          if (b.pin >= 0) _btnExpanded[i] = true;
        });

        el.innerHTML = _btnData.buttons
          .map((b, i) => {
            const exclude = [
              ...(_btnData.usedPins || []),
              ..._btnData.buttons
                .filter((_, j) => j !== i)
                .map((x) => x.pin)
                .filter((p) => p >= 0),
            ];

            if (!_btnExpanded[i]) {
              // Button 2 only appears if Button 1 exists
              // Button 3 only appears if Button 2 exists
              // Button 4 only appears if Button 3 exists
              const canAdd = i === 0 || _btnExpanded[i - 1];

              if (!canAdd) return "";

              return `
            <button type="button"
              class="btn-add-button"
              onclick="_expandBtn(${i})">
              + Add Button ${i + 1}
            </button>`;
            }

            return `
          ${i > 0 ? '<hr class="btn-config-separator">' : ""}
            <div class="btn-config-row">
              <div class="btn-config-header">
                <label class="btn-config-label">Button ${i + 1}</label>
                ${i > 0 ? `<button type="button" class="btn-config-remove" onclick="_collapseBtn(${i})">✕ Remove</button>` : ""}
              </div>
              <label class="btn-config-gpio-label">GPIO Pin:</label>
              <select id="btn${i + 1}_pin" onchange="_onBtnPinChange()">${_btnPinOpts(b.pin, exclude)}</select>
              <label class="btn-config-action-label">Short Press:</label>
              <select id="btn${i + 1}_short">${_btnActionOpts(b.shortAction)}</select>
              <label class="btn-config-action-label">Long Press (${BTN_LONG_MS}ms):</label>
              <select id="btn${i + 1}_long">${_btnActionOpts(b.longAction)}</select>
            </div>
          `;
          })
          .join("");
      }

      function _expandBtn(i) {
        _syncBtnData();
        _btnExpanded[i] = true;

        if (!_btnData.buttons[i]) {
          _btnData.buttons[i] = {
            pin: -1,
            shortAction: "",
            longAction: "",
          };
        }

        _renderBtnConfig();
      }

      async function _collapseBtn(i) {
        let needsSave = false;

        for (let j = i; j < 4; j++) {
          if (_btnData.buttons[j].pin >= 0) {
            needsSave = true;
          }

          _btnData.buttons[j] = {
            pin: -1,
            shortAction: "",
            longAction: "",
          };

          _btnExpanded[j] = false;
        }

        _renderBtnConfig();

        if (needsSave) {
          await saveButtonConfig(false);
        }
      }

      const BTN_LONG_MS = 800; // keep in sync with firmware

      function _onBtnPinChange() {
        if (!_btnData) return;
        for (let i = 0; i < 4; i++) {
          const p = document.getElementById(`btn${i + 1}_pin`);
          const s = document.getElementById(`btn${i + 1}_short`);
          const l = document.getElementById(`btn${i + 1}_long`);
          if (p) _btnData.buttons[i].pin = parseInt(p.value);
          if (s) _btnData.buttons[i].shortAction = s.value;
          if (l) _btnData.buttons[i].longAction = l.value;
        }
        _renderBtnConfig();
      }

      let btnSaveStatusTimer = null;

      function showBtnStatus(msg) {
        const el = document.getElementById("btn-save-status");
        el.textContent = msg;

        clearTimeout(btnSaveStatusTimer);

        btnSaveStatusTimer = setTimeout(() => {
          el.textContent = "";
        }, 3000);
      }

      async function saveButtonConfig(showStatus = true) {
        const statusEl = document.getElementById("btn-save-status");
        if (!_btnData) return;

        const params = new URLSearchParams();

        _syncBtnData();

        for (let i = 0; i < 4; i++) {
          const b = _btnData.buttons[i] || {};

          params.set(`btn${i + 1}_pin`, b.pin ?? -1);
          params.set(`btn${i + 1}_short`, b.shortAction ?? "");
          params.set(`btn${i + 1}_long`, b.longAction ?? "");
        }

        try {
          const res = await fetch("/save_buttons", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params,
          });

          if (showStatus) {
            showToast(
              res.ok ? "✓ Settings applied!" : "⚠️ \u00A0\u00A0Save failed.",
              res.ok ? 3000 : 5000,
            );
          }

          if (res.ok) {
            try {
              const buzzRes = await fetch("/get_buzzer");
              _buzzerData = await buzzRes.json();
              _renderBuzzerGlobal();
              _renderBuzzerEvents();
              setBuzzerFieldsEnabled(_buzzerData.enabled);
            } catch (e) {}
          }
        } catch {
          if (showStatus) {
            showToast("⚠️ \u00A0\u00A0Save failed.", 5000);
          }
        }
      }

      function setBuzzerFieldsEnabled(enabled) {
        document.getElementById("buzzerPin").disabled = !enabled;
        document.getElementById("buzzerVolumeSlider").disabled = !enabled;

        const testBtn = document.getElementById("buzzerTestBtn");
        testBtn.disabled = !enabled;
        testBtn.classList.toggle("geo-disabled", !enabled);

        const buzzerApplyBtn = document.getElementById("buzzerApplyBtn");
        buzzerApplyBtn.disabled = !enabled;
        buzzerApplyBtn.classList.toggle("geo-disabled", !enabled);

        document
          .querySelectorAll(
            '#buzzer-events-container select, #buzzer-events-container input[type="checkbox"]',
          )
          .forEach((el) => {
            el.disabled = !enabled;
          });
      }

      let _buzzerData = null;

      const BUZZER_EVENTS = [
        { idx: 1, label: "Countdown Finished" },
        { idx: 2, label: "Timer Finished" },
        { idx: 3, label: "Pomodoro Work Finished" },
        { idx: 4, label: "Pomodoro Break Finished" },
        { idx: 5, label: "Stopwatch" },
      ];
      // Alarm (0) and Button Feedback (6) intentionally left out — Alarm gets its own section later,
      // Button Feedback has no UI yet since we decided to skip it.

      const BUZZER_SOUND_OPTS = [
        { id: 1, label: "Beep" },
        { id: 2, label: "Chirp" },
        { id: 3, label: "Alarm" },
      ];

      function _buzzerSoundOpts(selected) {
        return BUZZER_SOUND_OPTS.map(
          (s) =>
            `<option value="${s.id}"${selected === s.id ? " selected" : ""}>${s.label}</option>`,
        ).join("");
      }

      function _renderBuzzerGlobal() {
        if (!_buzzerData) return;
        document.getElementById("buzzerEnabled").checked =
          !!_buzzerData.enabled;
        document.getElementById("buzzerVolumeSlider").value =
          _buzzerData.volume ?? 7;
        document.getElementById("buzzerVolumeValue").textContent =
          _buzzerData.volume ?? 7;

        const pinSel = document.getElementById("buzzerPin");
        const exclude = new Set((_buzzerData.usedPins || []).map(Number));
        let opts = `<option value="255"${_buzzerData.pin === 255 ? " selected" : ""}>- Disabled -</option>`;
        ALL_GPIO.forEach((p) => {
          if (!exclude.has(p))
            opts += `<option value="${p}"${_buzzerData.pin === p ? " selected" : ""}>GPIO ${p}</option>`;
        });
        pinSel.innerHTML = opts;
      }

      function _renderBuzzerEvents() {
        const el = document.getElementById("buzzer-events-container");
        if (!el || !_buzzerData) return;

        el.innerHTML = BUZZER_EVENTS.map((evt) => {
          const e = _buzzerData.events[evt.idx] || { enabled: true, sound: 1 };
          return `
          <div class="btn-config-action-label">
            <span class="label-text">${evt.label}:</span>
            <div class="toggle-row-lg select-and-toggle">
              <select id="evt${evt.idx}_sound" onchange="previewBuzzerSound(${evt.idx})">
                ${_buzzerSoundOpts(e.sound)}
              </select>
              <label class="toggle-switch">
                <input type="checkbox" id="evt${evt.idx}_enabled" ${e.enabled ? "checked" : ""} />
                <span class="toggle-slider"></span>
              </label>
            </div>
          </div>
          `;
        }).join("");
      }

      function previewBuzzerSound(idx) {
        const sel = document.getElementById(`evt${idx}_sound`);
        if (!sel) return;
        const vol = document.getElementById("buzzerVolumeSlider").value;
        const repeat = (_buzzerData.events[idx] || {}).repeat ? 1 : 0;
        fetch(`/action?play_sound=${sel.value}:${vol}:${repeat}`).catch(
          () => {},
        );
        if (repeat) {
          setTimeout(() => {
            fetch(`/action?buzzer_stop`).catch(() => {});
          }, 3000);
        }
      }

      function testBuzzer() {
        const vol = document.getElementById("buzzerVolumeSlider").value;
        fetch(`/action?play_sound=3:${vol}:1`).catch(() => {});
        setTimeout(() => {
          fetch(`/action?buzzer_stop`).catch(() => {});
        }, 3000);
      }

      let buzzerSaveStatusTimer = null;
      function showBuzzerStatus(msg) {
        const el = document.getElementById("buzzer-save-status");
        el.textContent = msg;
        clearTimeout(buzzerSaveStatusTimer);
        buzzerSaveStatusTimer = setTimeout(() => {
          el.textContent = "";
        }, 3000);
      }

      async function saveBuzzerConfig(showStatus = true) {
        if (!_buzzerData) return;
        const statusEl = document.getElementById("buzzer-save-status");
        const params = new URLSearchParams();

        params.set("pin", document.getElementById("buzzerPin").value);
        params.set(
          "enabled",
          document.getElementById("buzzerEnabled").checked ? "1" : "0",
        );
        params.set(
          "volume",
          document.getElementById("buzzerVolumeSlider").value,
        );

        BUZZER_EVENTS.forEach((evt) => {
          const en = document.getElementById(`evt${evt.idx}_enabled`);
          const sd = document.getElementById(`evt${evt.idx}_sound`);
          if (en) params.set(`evt${evt.idx}_enabled`, en.checked ? "1" : "0");
          if (sd) params.set(`evt${evt.idx}_sound`, sd.value);
        });

        try {
          const res = await fetch("/save_buzzer", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params,
          });
          if (showStatus)
            showToast(
              res.ok ? "✓ Settings applied" : "⚠️ \u00A0\u00A0Save failed.",
              res.ok ? 3000 : 5000,
            );
          if (res.ok) {
            try {
              const btnRes = await fetch("/get_buttons");
              _btnData = await btnRes.json();
              _renderBtnConfig();
            } catch (e) {}
          }
        } catch {
          if (showStatus) {
            showToast("⚠️ \u00A0\u00A0Save failed.", 5000);
          }
        }
      }
      let _alarmData = null;
      let _alarmExpanded = [true, false, false, false];

      const ALARM_SOUND_OPTS = [
        { id: 1, label: "Beep" },
        { id: 2, label: "Chirp" },
        { id: 3, label: "Alarm" },
      ];

      function _alarmSoundOpts(selected) {
        return ALARM_SOUND_OPTS.map(
          (s) =>
            `<option value="${s.id}"${selected === s.id ? " selected" : ""}>${s.label}</option>`,
        ).join("");
      }

      function _alarmScheduleText(a) {
        if (!a.enabled) return "NO ALARM SET";
        const timeVal =
          String(a.hour).padStart(2, "0") +
          ":" +
          String(a.minute).padStart(2, "0");
        const dayLabels = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"];
        const selected = [];
        for (let i = 0; i < 7; i++) if (a.days[i]) selected.push(dayLabels[i]);
        if (selected.length === 0) return `ALARM ${timeVal} (NO DAYS SELECTED)`;
        if (selected.length === 7) return `ALARM ${timeVal} EVERY DAY`;
        return `ALARM ${timeVal} ${selected.join(" ")}`;
      }

      function _renderAlarmConfig() {
        const el = document.getElementById("alarm-config-container");
        if (!el || !_alarmData) return;

        _alarmData.alarms.forEach((a, i) => {
          if (a.enabled) _alarmExpanded[i] = true;
        });

        el.innerHTML = _alarmData.alarms
          .map((a, i) => {
            if (!_alarmExpanded[i]) {
              const canAdd = i === 0 || _alarmExpanded[i - 1];
              if (!canAdd) return "";
              return `
        <button type="button" class="btn-add-button" onclick="_expandAlarm(${i})">
          + Add Alarm ${i + 1}
        </button>`;
            }

            return `
      ${i > 0 ? '<hr class="btn-config-separator">' : ""}
      <div class="btn-config-row">
        <div class="btn-config-header">
          <label class="btn-config-label">Alarm ${i + 1}</label>
          ${i > 0 ? `<button type="button" class="btn-config-remove" onclick="_collapseAlarm(${i})">✕ Remove</button>` : ""}
        </div>

        <label class="toggle-row-lg">
          <span class="label-text">Enable Alarm:</span>
          <span class="toggle-switch">
            <input type="checkbox" id="alarm${i}_enabled" ${a.enabled ? "checked" : ""} onchange="_updateAlarmPreview(${i})" />
            <span class="toggle-slider"></span>
          </span>
        </label>

        <label>Alarm ${i + 1} Brightness: <span id="alarm${i}_brightnessValue">${a.brightness}</span></label>
        <input class="range-full" type="range" min="0" max="15" id="alarm${i}_brightness" value="${a.brightness}"
          oninput="document.getElementById('alarm${i}_brightnessValue').textContent = this.value;" />

        <label for="alarm${i}_time">Time:</label>
        <input type="time" id="alarm${i}_time" value="${String(a.hour).padStart(2, "0")}:${String(a.minute).padStart(2, "0")}" oninput="_updateAlarmPreview(${i})" />

        <label>Repeat:</label>
        <div class="alarm-days-row" id="alarm${i}_daysRow">
          ${["Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"]
            .map(
              (lbl, d) => `
            <label class="day-pill${a.days[d] ? " checked" : ""}" data-day="${d}">
              <input type="checkbox" id="alarm${i}_day${d}" ${a.days[d] ? "checked" : ""} onchange="this.closest('.day-pill').classList.toggle('checked', this.checked); _updateAlarmPreview(${i});" />${lbl}
            </label>`,
            )
            .join("")}
        </div>

        <label for="alarm${i}_sound">Sound:</label>
        <select id="alarm${i}_sound" onchange="previewAlarmSound(${i})">
          ${_alarmSoundOpts(a.sound)}
        </select>

        <label for="alarm${i}_snooze">Snooze duration (minutes):</label>
        <input type="number" id="alarm${i}_snooze" min="1" max="60" value="${a.snoozeMinutes}" />

        <p id="alarm${i}_previewText" class="alarm-preview-text">${_alarmScheduleText(a)}</p>

        <div class="btn-apply-wrap">
          <button type="button" id="alarm${i}_testBtn" class="primary-button cmsg1 btn-apply-top" onclick="testAlarm(${i})">
            Test Alarm
          </button>
        </div>
      </div>
    `;
          })
          .join("");

        for (let i = 0; i < 4; i++) {
          if (_alarmExpanded[i])
            setAlarmFieldsEnabled(i, _alarmData.alarms[i].enabled);
        }
      }

      function _expandAlarm(i) {
        _syncAlarmData();
        _alarmExpanded[i] = true;
        if (!_alarmData.alarms[i]) {
          _alarmData.alarms[i] = {
            enabled: false,
            hour: 7,
            minute: 0,
            days: [true, true, true, true, true, true, true],
            snoozeMinutes: 15,
            brightness: 10,
            sound: 3,
          };
        }
        _renderAlarmConfig();
      }

      async function _collapseAlarm(i) {
        _syncAlarmData();
        const needsSave = _alarmData.alarms[i].enabled;
        _alarmData.alarms[i] = {
          enabled: false,
          hour: 7,
          minute: 0,
          days: [true, true, true, true, true, true, true],
          snoozeMinutes: 15,
          brightness: 10,
          sound: 3,
        };
        _alarmExpanded[i] = false;
        _renderAlarmConfig();
        if (needsSave) await saveAlarmConfig(false);
      }

      function _syncAlarmData() {
        if (!_alarmData) return;
        for (let i = 0; i < 4; i++) {
          if (!_alarmExpanded[i]) continue;
          const enabledEl = document.getElementById(`alarm${i}_enabled`);
          if (!enabledEl) continue;
          const timeEl = document.getElementById(`alarm${i}_time`);
          const [h, m] = (timeEl.value || "07:00").split(":");

          _alarmData.alarms[i].enabled = enabledEl.checked;
          _alarmData.alarms[i].hour = parseInt(h);
          _alarmData.alarms[i].minute = parseInt(m);
          for (let d = 0; d < 7; d++) {
            _alarmData.alarms[i].days[d] = document.getElementById(
              `alarm${i}_day${d}`,
            ).checked;
          }
          _alarmData.alarms[i].sound = parseInt(
            document.getElementById(`alarm${i}_sound`).value,
          );
          _alarmData.alarms[i].brightness = parseInt(
            document.getElementById(`alarm${i}_brightness`).value,
          );
          _alarmData.alarms[i].snoozeMinutes = parseInt(
            document.getElementById(`alarm${i}_snooze`).value,
          );
        }
      }

      function _updateAlarmPreview(i) {
        const enabledEl = document.getElementById(`alarm${i}_enabled`);
        const timeEl = document.getElementById(`alarm${i}_time`);
        if (!enabledEl || !timeEl) return;
        const enabled = enabledEl.checked;
        const timeVal = timeEl.value || "00:00";
        const dayLabels = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"];
        let selected = [];
        for (let d = 0; d < 7; d++) {
          const cb = document.getElementById(`alarm${i}_day${d}`);
          if (cb && cb.checked) selected.push(dayLabels[d]);
        }
        let text;
        if (!enabled) text = "NO ALARM SET";
        else if (selected.length === 0)
          text = `ALARM ${timeVal} (NO DAYS SELECTED)`;
        else if (selected.length === 7) text = `ALARM ${timeVal} EVERY DAY`;
        else text = `ALARM ${timeVal} ${selected.join(" ")}`;
        document.getElementById(`alarm${i}_previewText`).textContent = text;
        setAlarmFieldsEnabled(i, enabled);
      }

      function setAlarmFieldsEnabled(i, enabled) {
        [
          `alarm${i}_time`,
          `alarm${i}_brightness`,
          `alarm${i}_sound`,
          `alarm${i}_snooze`,
        ].forEach((id) => {
          const el = document.getElementById(id);
          if (el) el.disabled = !enabled;
        });
        for (let d = 0; d < 7; d++) {
          const cb = document.getElementById(`alarm${i}_day${d}`);
          if (cb) {
            cb.disabled = !enabled;
            cb.closest(".day-pill").classList.toggle("disabled", !enabled);
          }
        }
        const testBtn = document.getElementById(`alarm${i}_testBtn`);
        if (testBtn) {
          testBtn.disabled = !enabled;
          testBtn.classList.toggle("geo-disabled", !enabled);
        }
      }

      let _alarmTestRinging = [false, false, false, false];
      function testAlarm(i) {
        const btn = document.getElementById(`alarm${i}_testBtn`);
        const actionPrefix = i === 0 ? "alarm" : `alarm${i + 1}`;
        if (_alarmTestRinging[i]) {
          fetch(`/action?${actionPrefix}_stop`).catch(() => {});
          _alarmTestRinging[i] = false;
          btn.textContent = "Test Alarm";
        } else {
          for (let j = 0; j < 4; j++) {
            if (j !== i && _alarmTestRinging[j]) {
              _alarmTestRinging[j] = false;
              const otherBtn = document.getElementById(`alarm${j}_testBtn`);
              if (otherBtn) otherBtn.textContent = "Test Alarm";
            }
          }
          const brightness = document.getElementById(
            `alarm${i}_brightness`,
          ).value;

          const sound = document.getElementById(`alarm${i}_sound`).value;
          fetch(`/action?${actionPrefix}_test=${brightness}:${sound}`).catch(
            () => {},
          );
          _alarmTestRinging[i] = true;
          btn.textContent = "Stop Test";
        }
      }

      function previewAlarmSound(i) {
        const sel = document.getElementById(`alarm${i}_sound`);
        if (!sel) return;
        fetch(`/action?play_sound=${sel.value}`).catch(() => {});
      }

      let alarmSaveStatusTimer = null;
      function showAlarmStatus(msg) {
        const el = document.getElementById("alarm-save-status");
        el.textContent = msg;
        clearTimeout(alarmSaveStatusTimer);
        alarmSaveStatusTimer = setTimeout(() => {
          el.textContent = "";
        }, 3000);
      }

      async function saveAlarmConfig(showStatus = true) {
        if (!_alarmData) return;

        // Stop any active alarm test before saving.
        let alarmTestWasActive = false;

        for (let i = 0; i < 4; i++) {
          if (_alarmTestRinging[i]) {
            alarmTestWasActive = true;
            const actionPrefix = i === 0 ? "alarm" : `alarm${i + 1}`;
            fetch(`/action?${actionPrefix}_stop`).catch(() => {});
            _alarmTestRinging[i] = false;

            const btn = document.getElementById(`alarm${i}_testBtn`);
            if (btn) btn.textContent = "Test Alarm";
          }
        }

        // Give the ESP8266 time to process the stop before saving.
        if (alarmTestWasActive) {
          await new Promise((resolve) => setTimeout(resolve, 500));
        }

        _syncAlarmData();
        const statusEl = document.getElementById("alarm-save-status");
        const params = new URLSearchParams();

        for (let i = 0; i < 4; i++) {
          const a = _alarmData.alarms[i];
          const p = `alarm${i}_`;
          params.set(p + "enabled", a.enabled ? "1" : "0");
          params.set(p + "hour", a.hour);
          params.set(p + "minute", a.minute);
          for (let d = 0; d < 7; d++)
            params.set(`${p}day${d}`, a.days[d] ? "1" : "0");
          params.set(p + "snoozeMinutes", a.snoozeMinutes);
          params.set(p + "brightness", a.brightness);
          params.set(p + "sound", a.sound);
        }

        try {
          const res = await fetch("/save_alarm", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params,
          });
          if (showStatus)
            showToast(
              res.ok ? "✓ Settings applied" : "⚠️ \u00A0\u00A0Save failed.",
              res.ok ? 3000 : 5000,
            );
        } catch {
          if (showStatus) showToast("⚠️ \u00A0\u00A0Save failed.", 5000);
        }
      }

      // ---------- Weather ----------
      let weatherSaveStatusTimer = null;
      function showWeatherStatus(msg) {
        const el = document.getElementById("weather-save-status");
        el.textContent = msg;
        clearTimeout(weatherSaveStatusTimer);
        weatherSaveStatusTimer = setTimeout(() => {
          el.textContent = "";
        }, 3000);
      }

      async function saveWeatherConfig(showStatus = true) {
        const statusEl = document.getElementById("weather-save-status");
        const params = new URLSearchParams();

        params.set(
          "weatherDuration",
          parseInt(document.getElementById("weatherDuration").value, 10) * 1000,
        );

        // Reuse the same masked-key skip logic as submitConfig() so Apply
        // never overwrites a saved key with the "****" placeholder.
        const apiKeyToSend = apiInput.value;
        if (!(apiKeyToSend === MASK && hasSavedKey)) {
          params.set("openWeatherApiKey", apiKeyToSend);
        }

        params.set(
          "openWeatherCity",
          document.getElementById("openWeatherCity").value,
        );
        params.set(
          "openWeatherCountry",
          document.getElementById("openWeatherCountry").value,
        );
        params.set(
          "weatherUnits",
          document.getElementById("weatherUnits").checked
            ? "imperial"
            : "metric",
        );
        params.set(
          "showHumidity",
          document.getElementById("showHumidity").checked ? "on" : "",
        );
        params.set(
          "showWeatherDescription",
          document.getElementById("showWeatherDescription").checked ? "on" : "",
        );

        try {
          const res = await fetch("/save_weather", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params,
          });
          showToast(
            res.ok ? "✓ Settings applied" : "⚠️ \u00A0\u00A0Save failed.",
            res.ok ? 3000 : 5000,
          );
        } catch {
          showToast("⚠️ \u00A0\u00A0Save failed.", 5000);
        }
      }

      // ---------- Time & Date ----------
      let timeDateSaveStatusTimer = null;
      function showTimeDateStatus(msg) {
        const el = document.getElementById("timedate-save-status");
        el.textContent = msg;
        clearTimeout(timeDateSaveStatusTimer);
        timeDateSaveStatusTimer = setTimeout(() => {
          el.textContent = "";
        }, 3000);
      }

      async function saveTimeDateConfig(showStatus = true) {
        const statusEl = document.getElementById("timedate-save-status");
        const params = new URLSearchParams();

        params.set("timeZone", document.getElementById("timeZone").value);
        params.set(
          "clockDuration",
          parseInt(document.getElementById("clockDuration").value, 10) * 1000,
        );
        params.set("ntpServer1", document.getElementById("ntpServer1").value);
        params.set("ntpServer2", document.getElementById("ntpServer2").value);
        params.set(
          "showDayOfWeek",
          document.getElementById("showDayOfWeek").checked ? "on" : "",
        );
        params.set(
          "colonBlinkEnabled",
          document.getElementById("colonBlinkEnabled").checked ? "on" : "",
        );
        params.set(
          "showDate",
          document.getElementById("showDate").checked ? "on" : "",
        );
        params.set(
          "twelveHourToggle",
          document.getElementById("twelveHourToggle").checked ? "on" : "",
        );

        try {
          const res = await fetch("/save_timedate", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params,
          });
          showToast(
            res.ok ? "✓ Settings applied" : "⚠️  Save failed.",
            res.ok ? 3000 : 5000,
          );
        } catch {
          showToast("⚠️ \u00A0\u00A0Save failed.", 5000);
        }
      }

      // ---------- Display ----------
      let displaySaveStatusTimer = null;
      function showDisplayStatus(msg) {
        const el = document.getElementById("display-save-status");
        el.textContent = msg;
        clearTimeout(displaySaveStatusTimer);
        displaySaveStatusTimer = setTimeout(() => {
          el.textContent = "";
        }, 3000);
      }

      async function saveDisplayConfig(showStatus = true) {
        const statusEl = document.getElementById("display-save-status");
        const params = new URLSearchParams();

        params.set(
          "brightness",
          document.getElementById("brightnessSlider").value,
        );
        params.set(
          "flipDisplay",
          document.getElementById("flipDisplay").checked ? "on" : "",
        );

        // Same mutual-exclusivity rule as submitConfig()
        const autoDimmingChecked =
          document.getElementById("autoDimmingEnabled").checked;
        const customDimmingChecked =
          document.getElementById("dimmingEnabled").checked;
        if (autoDimmingChecked && customDimmingChecked) {
          params.set("autoDimmingEnabled", "true");
          params.set("dimmingEnabled", "false");
        } else {
          params.set(
            "autoDimmingEnabled",
            autoDimmingChecked ? "true" : "false",
          );
          params.set("dimmingEnabled", customDimmingChecked ? "true" : "false");
        }

        const dimStart = document.getElementById("dimStartTime").value; // "18:45"
        const dimEnd = document.getElementById("dimEndTime").value; // "08:30"
        if (dimStart) {
          const [startHour, startMin] = dimStart
            .split(":")
            .map((x) => parseInt(x, 10));
          params.set("dimStartHour", startHour);
          params.set("dimStartMinute", startMin);
        }
        if (dimEnd) {
          const [endHour, endMin] = dimEnd
            .split(":")
            .map((x) => parseInt(x, 10));
          params.set("dimEndHour", endHour);
          params.set("dimEndMinute", endMin);
        }

        params.set(
          "dimBrightness",
          document.getElementById("dimBrightness").value,
        );
        params.set(
          "clockOnlyDuringDimming",
          document.getElementById("clockOnlyDuringDimming").checked ? "on" : "",
        );

        try {
          const res = await fetch("/save_display", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params,
          });
          showToast(
            res.ok ? "✓ Settings applied" : "⚠️ Save failed.",
            res.ok ? 3000 : 5000,
          );
        } catch {
          showToast("⚠️ \u00A0\u00A0Save failed.", 5000);
        }
      }

      // ---------- Countdown ----------
      let countdownSaveStatusTimer = null;
      function showCountdownStatus(msg) {
        const el = document.getElementById("countdown-save-status");
        el.textContent = msg;
        clearTimeout(countdownSaveStatusTimer);
        countdownSaveStatusTimer = setTimeout(() => {
          el.textContent = "";
        }, 3000);
      }

      async function saveCountdownConfig(showStatus = true) {
        const statusEl = document.getElementById("countdown-save-status");
        const params = new URLSearchParams();

        params.set(
          "countdownEnabled",
          document.getElementById("countdownEnabled").checked ? "on" : "",
        );
        params.set(
          "isDramaticCountdown",
          document.getElementById("isDramaticCountdown").checked ? "on" : "",
        );
        params.set(
          "countdownDate",
          document.getElementById("countdownDate").value,
        );
        params.set(
          "countdownTime",
          document.getElementById("countdownTime").value,
        );
        params.set(
          "countdownLabel",
          document.getElementById("countdownLabel").value,
        );

        try {
          const res = await fetch("/save_countdown", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params,
          });

          if (showStatus) {
            showToast(
              res.ok ? "✓ Countdown applied" : "⚠️ Save failed.",
              res.ok ? 3000 : 5000,
            );
          }
        } catch {
          if (showStatus) {
            showToast("⚠️ \u00A0\u00A0Save failed.", 5000);
          }
        }
      }

      function quickAction(action, value = "") {
        fetch(`http://${deviceIP}/action`, {
          method: "POST",
          headers: {
            "Content-Type": "application/x-www-form-urlencoded",
            "X-Source": "UI",
          },
          body: new URLSearchParams({
            [action]: value,
          }),
        }).catch((e) => {
          console.error("Quick action failed:", e);
        });
      }

      function startQuickTimer() {
        const input = document.getElementById("quickTimer");
        const value = input.value.trim();

        if (!value) {
          input.focus();
          return;
        }

        quickAction("timer", value);
      }

      let toastTimeout;

      function showToast(message, duration = 2000) {
        const toast = document.getElementById("toast");

        if (!toast) return;

        clearTimeout(toastTimeout);

        toast.textContent = message;
        toast.classList.add("show");

        toastTimeout = setTimeout(() => {
          toast.classList.remove("show");
        }, duration);
      }
    </script>
    <!--
    Third-party component: Lucide Icons
    Copyright (c) 2026 Lucide Icons and Contributors
    Permission to use, copy, modify, and/or distribute this software for any purpose
    with or without fee is hereby granted, provided that the above copyright notice
    and this permission notice appear in all copies.
    Licensed under the ISC License.
    https://lucide.dev/license
    -->
  </body>
</html>
)rawliteral";