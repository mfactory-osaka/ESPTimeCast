#pragma once
// index_html.h

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
<meta http-equiv="Pragma" content="no-cache" />
<meta http-equiv="Expires" content="0" />
<title>ESPTimeCast Settings</title>
<style>

:root{
  --accent-color: #0075ff;
}

  * { box-sizing: border-box; }
  html{
    background: radial-gradient(ellipse at 70% 0%, #2b425a 0%, #171e23 100%);  
    height: 100%;
  }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, "Helvetica Neue", sans-serif;
    margin: 0;
    padding: 2rem 1rem;
    color: #FFFFFF;
    background-repeat: no-repeat, repeat, repeat; 
    opacity: 0;
    transition: opacity 0.6s cubic-bezier(.4,0,.2,1);
    visibility: hidden;
    height: 100%;
    line-height: 1.5;
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
  h2{
    margin-top: 2rem;
    margin-bottom: 0;
  }

  h2:first-of-type{
    margin-top: 1.5rem;
  }

  .logo svg{
    filter: drop-shadow(0px 0px 0.5rem #1ec7fa);
    width: 100%;
    height: auto;
    color: #c2f0ff;
    margin: 0.5rem 0;
  }
  form {
    display: flex; flex-direction: column;
    max-width: 500px; margin: 0 auto;
    background: linear-gradient(120deg, rgba(45,65,90,0.72) 0%, rgba(53,133,183,0.38) 100%); 
    padding: 1.5rem;
    border-radius: 24px;
    box-shadow: 0 10px 36px 0 rgba(40,170,255,0.11), 0 2px 8px 0 rgba(44, 70, 110, 0.08);
    border: 1.5px solid rgba(180, 230, 255, 0.10)
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
  input[type="number"], select {
    width: 100%; padding: 0.75rem;
    border: 1.5px solid rgba(180, 230, 255, 0.08);
    border-radius: 8px;
    background-color: rgba(225,245,255,0.07); 
    color: #ffffff;
    font-size: 1rem; appearance: none;
  }

  input[type="time"]:disabled ,
  input[type="text"]:disabled ,
  input[type="number"]:disabled ,
  input[type="date"]:disabled{
    color: rgba(255, 255, 255, 0.250);
  }

  input[type="submit"] {
    background-color: #007aff; color: white;
    padding: 0.9rem; font-size: 1rem;
    border: none; border-radius: 8px;
    cursor: pointer; transition: background-color 0.2s ease-in-out;
  }
  input[type="submit"]:hover {
    background-color: #005ecb;
  }

input[type="time"]::-webkit-calendar-picker-indicator, input[type="date"]::-webkit-calendar-picker-indicator{
  filter: invert(100%);
}


/* Enabled & checked toggle */
.toggle-switch input[type="checkbox"]:checked + .toggle-slider {
  background-color: var(--accent-color);
}

/* Disabled toggle (regardless of checked state) */
.toggle-switch input[type="checkbox"]:disabled + .toggle-slider { background-color: transparent !important; border: solid 1px #cccccc2e; cursor: not-allowed; }
.toggle-switch input[type="checkbox"]:disabled + .toggle-slider::before {
  background-color: rgba(204, 204, 204, 0.5);
}


  input:-webkit-autofill,
input:-webkit-autofill:focus,
input:-webkit-autofill:hover {
  background: rgba(225,245,255,0.07) !important;
  color: #fff !important;
  -webkit-box-shadow: 0 0 0 1000px rgba(225,245,255,0.07) inset !important;
  box-shadow: 0 0 0 1000px rgba(225,245,255,0.07) inset !important;
  -webkit-text-fill-color: #fff !important;
  transition: background 9999s ease-in-out 0s;
}

input::placeholder, 
textarea::placeholder {
  color: hwb(0 100% 0% / 0.39);      /* Example: light blue */
  opacity: 1;          /* Make sure it's not semi-transparent */
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
    background: linear-gradient(90deg, #3e99bc, #47add4 85%);
    color: white;
    padding: 0.9rem; font-size: 1rem; font-weight: 600;
    border: none; border-radius: 8px;
    cursor: pointer; text-align: center;
    transition: background 0.25s, transform 0.15s ease-in-out;
  }
  .primary-button:hover {
    transform: translateY(-1px);
    box-shadow: 0 6px 16px rgba(0, 122, 255, 0.35);
  }
  .primary-button:active {
    transform: scale(0.97); 
  }  

  .note { 
    font-size: 0.85rem; 
    text-align: center; 
    margin-top: 1rem; 
  }
  #savingModal {
    backdrop-filter: blur(5px);
    position: fixed; top: 0; left: 0;
    width: 100%; height: 100%;
    background: radial-gradient(ellipse at 70% 0%, hsl(210.64deg 35.34% 26.08% / 60%) 0%, #171e2399 100%);
    display: none; justify-content: center; align-items: center;
    z-index: 1000;
  }
  #savingModalContent {
    background: linear-gradient(120deg, rgb(45 65 90) 0%, rgb(53 133 183 / 44%) 100%);
    border-radius: 12px;
    box-shadow: 0 10px 36px 0 rgba(40, 170, 255, 0.11), 0 2px 8px 0 rgba(44, 70, 110, 0.08);
    border: 1.5px solid rgba(180, 230, 255, 0.10);
    margin: 1.5rem;
    padding: 2rem 2.5rem;
    text-align: center;
  }  
  .spinner {
    border: 4px solid rgba(255, 255, 255, 0.2);
    border-top: 4px solid #007aff;
    border-radius: 50%;
    width: 36px; height: 36px;
    animation: spin 1s linear infinite;
    margin: 0 auto 1rem;
  }
  .footer{
    font-size: 0.8rem;
    text-align: center;
    margin-top: 1rem;
  }
  a{
    color: white;
  }

  .small{
    display: block;
    font-size: 0.8rem;
    margin-top: 0.25rem;
  }

  select option {
    color: black;
  }
  
  .geo-disabled{
    opacity: 0.5;
    background: transparent;
    border: 0.1rem white solid;
    cursor: not-allowed;
  }

  .geo-disabled:hover{
    transform: none;
    box-shadow: none;
  }

 .button-row {
    display: flex;
    margin-top: 0.5rem;
    gap: 0.8rem;
    justify-content: center;
}

  @keyframes spin {
    0% { transform: rotate(0deg); }
    100% { transform: rotate(360deg); }
  }

      #openWeatherCountry{
        margin-top: 0.75rem;
      }

  @media (min-width: 361px) {
    .form-row.two-col { 
      flex-direction: row; 
      gap: 1rem; }

      #openWeatherCountry{
        margin-top: 0;
      }
  }




/* Make sure .details-content is visible only when open */
.animated-details[open] .details-content {
  /* JS will animate the height */
}


  /* Toggle Switch Styling for Flip Display */
  .toggle-switch { position: relative; display: inline-block; width: 48px; height: 24px; }
  .toggle-switch input { opacity: 0; width: 0; height: 0; }
  .toggle-slider {
    position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
    background-color: #ccc; transition: .4s; border-radius: 24px;
  }
  .toggle-slider:before {
    position: absolute; content: ""; height: 20px; width: 20px; left: 2px; bottom: 2px;
    background-color: white; transition: .4s; border-radius: 50%;
  }
  input:checked + .toggle-slider { background-color: var(--accent-color); }
  input:checked + .toggle-slider:before { transform: translateX(24px); }

  .accent{
    accent-color: var(--accent-color);
  }

      .collapsible-toggle {
    display: flex;
    align-items: center;
    cursor: pointer;
    font-size: 1.1rem;
    font-weight: bold;
    background: none;
    border: none;
    color: white;
    padding: 0;
    margin: 0;
    outline: none;
    gap: 0.5em;
    user-select: none;
    margin-top: 2.5rem;
    }
    .collapsible-toggle .icon-area {
      transition: transform 0.3s cubic-bezier(.4,0,.2,1);
      display: flex;
    }
    .collapsible-toggle.open .icon-area {
      transform: rotate(90deg);
    }
    .collapsible-content {
    overflow: hidden;
    height: 0;
    transition: height 0.3s cubic-bezier(.4,0,.2,1);
    color: #fff;
    margin-bottom: 2rem;
    }
    .collapsible-content-inner {
      padding: 1em 0;
    }
</style>
</head>
<body>

<form id="configForm" onsubmit="submitConfig(event)">
  <div class="logo">
    <svg width="450" height="50" viewBox="0 0 119.063 13.229" xml:space="preserve" xmlns="http://www.w3.org/2000/svg"><path style="fill:currentColor;font-variation-settings:&quot;wdth&quot;87,&quot;wght&quot;700;paint-order:markers fill stroke" d="M.75 0a.75.75 0 1 0-.002 1.495A.75.75 0 0 0 .75 0m1.957 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.082 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0-.002 1.495A.75.75 0 0 0 24.654 0m1.957 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m1.956 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.126 0a.75.75 0 1 0-.002 1.495A.75.75 0 0 0 48.558 0m29.987 0a.75.75 0 1 0-.002 1.495A.75.75 0 0 0 78.545 0M80.5 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498M114.4 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498M.75 1.955a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .002-1.5m11.952 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m7.826 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 .001-1.498m4.126 0a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .002-1.5m7.826 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 .001-1.498m8.039 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m36.069 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m7.826 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m29.986 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498M.75 3.91a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m11.952 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m11.952 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m7.826 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.039 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.04 0a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .001-1.5m4.125 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.039 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m13.908 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 .001-1.5m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.125 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498M.75 5.866a.75.75 0 1 0-.002 1.499.75.75 0 0 0 .002-1.5m1.957 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m1.956 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m1.956 0a.749.749 0 1 0 0 1.497.75.75 0 1 0 0-1.498m8.039 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m1.957 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m1.956 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m6.083 0a.75.75 0 1 0-.002 1.499.75.75 0 0 0 .002-1.5m1.957 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m1.956 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 .001-1.498m1.956 0a.749.749 0 1 0 0 1.497.75.75 0 1 0 0-1.498m9.996 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m8.04 0a.75.75 0 1 0-.003 1.499.75.75 0 0 0 .002-1.5m4.125 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m3.913 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m3.913 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m4.125 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m7.827 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m4.126 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m11.951 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m7.827 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m4.126 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m13.908 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497M.75 7.82a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m19.778 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.126 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m15.865 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.04 0a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .001-1.5m4.125 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.5m3.913 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m3.913 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.125 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.126 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.5m11.951 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m7.827 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.5m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 .001-1.5m8.038 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498M.75 9.776a.75.75 0 1 0-.002 1.499.75.75 0 0 0 .002-1.499m11.952 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m7.826 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m4.126 0a.75.75 0 1 0-.002 1.499.75.75 0 0 0 .002-1.499m15.865 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m8.04 0a.75.75 0 1 0-.003 1.499.75.75 0 0 0 .002-1.499m4.125 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m3.913 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m3.913 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m4.125 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m11.953 0a.749.749 0 1 0-.002 1.498.749.749 0 0 0 .002-1.498m7.826 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m4.125 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m5.87 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m1.957 0a.749.749 0 1 0-.001 1.497.749.749 0 0 0 .001-1.497m11.952 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497m6.082 0a.749.749 0 1 0-.002 1.497.749.749 0 0 0 .002-1.497M.75 11.731a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m1.957 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.082 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.083 0a.75.75 0 1 0 0 1.5.75.75 0 0 0 0-1.5m15.865 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.04 0a.75.75 0 1 0-.002 1.5.75.75 0 0 0 .001-1.5m4.125 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m3.913 0a.749.749 0 1 0-.001 1.498.749.749 0 0 0 0-1.498m3.913 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m6.082 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.04 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.499m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m8.039 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.957 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m3.913 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m4.126 0a.75.75 0 1 0-.001 1.499.75.75 0 0 0 0-1.499m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 0-1.5m1.956 0a.75.75 0 1 0 0 1.499.75.75 0 0 0 0-1.499m1.957 0a.75.75 0 1 0-.001 1.5.75.75 0 0 0 .001-1.5m9.995 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498m1.956 0a.749.749 0 1 0 0 1.498.749.749 0 0 0 0-1.498"/></svg>
  </div>
  <h2>WiFi Settings</h2>
  <label for="ssid">SSID</label>
  <input type="text" id="ssid" name="ssid" required />
  <label for="password">Password</label>
  <div style="position: relative;">
    <input type="password" id="password" name="password" required />
    <label class="small">
      <input type="checkbox" id="togglePassword" style="margin-right: 0.3rem;" />
      Show Password
    </label>
  </div>


  <h2>Clock Settings</h2>
  <label for="timeZone">Time Zone</label>
  <select id="timeZone" name="timeZone" required>
    <option value="" disabled selected>Select your time zone</option>
  <option value="Africa/Cairo">Africa/Cairo</option>
  <option value="Africa/Casablanca">Africa/Casablanca</option>
  <option value="Africa/Johannesburg">Africa/Johannesburg</option>
  <option value="America/Anchorage">America/Anchorage</option>
  <option value="America/Argentina/Buenos_Aires">America/Argentina/Buenos_Aires</option>
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
  <option value="Asia/Tokyo">Asia/Tokyo</option>
  <option value="Asia/Ulaanbaatar">Asia/Ulaanbaatar</option>
  <option value="Asia/Yekaterinburg">Asia/Yekaterinburg</option>
  <option value="Atlantic/Azores">Atlantic/Azores</option>
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

    <label for="language">Language (Day & Weather)</label>
  <select id="language" name="language" onchange="setLanguage(this.value)">
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

  <div class="form-row two-col">
    <div>
      <label for="clockDuration">Clock Duration</label>
      <input type="number" id="clockDuration" name="clockDuration" min="1" required />
      <label class="small">(Seconds)</label>
    </div>
    <div>
      <label for="weatherDuration">Weather Duration</label>
      <input type="number" id="weatherDuration" name="weatherDuration" min="1" required />
      <label class="small">(Seconds)</label>
    </div>
  </div>


  <h2>Weather Settings</h2>
  <label for="openWeatherApiKey">OpenWeather API Key</label>
  <input type="text" id="openWeatherApiKey" name="openWeatherApiKey" placeholder="ADD-YOUR-API-KEY-32-CHARACTERS"/>    
  <div class="small">Required to fetch weather data. <a href="https://home.openweathermap.org/users/sign_up" target="_blank">Get your API key here</a>.</div>

  <label>Location</label>
  <div class="form-row two-col"> 
    <input type="text" id="openWeatherCity" name="openWeatherCity" placeholder="City / Zip / Lat."/>    
    <input type="text" id="openWeatherCountry" name="openWeatherCountry" placeholder="Country Code / Long."/>  
  </div>
  <button type="button" class="primary-button" id="geo-button" onclick="getLocation()" style="margin-top: 1rem;">Get My Location</button>

  <div class="small">    
    <strong>Location format examples:</strong> City, Country Code - Osaka, JP | ZIP, Country Code - 94040, US | Latitude, Longitude - 34.6937, 135.5023
  </div>
  
  <div class="form-group">
    <h2>Custom Message</h2>
    <input id="customMessage" name="customMessage" type="text" maxlength="120" placeholder="ENTER MESSAGE" pattern="[A-Z0-9 :!'\-.,_\+%\/?]*" title="Only uppercase letters, numbers, space, and : ! ' - . ? , _ + % / ? allowed">
    <div class="small">Allowed characters: A–Z, 0–9, space, and : ! ' - . ? , _ + % /</div>
  </div>
  <div class="button-row">
    <button type="button" class="primary-button cmsg1" onclick="sendCustomMessage()">Send Message</button>
    <button type="button" class="primary-button cmsg2" onclick="clearCustomMessage()">Clear Message</button>
  </div>     

  <div class="geo-note" style="display: none;">
    <br>
    <span class="small"><strong>Note:</strong> External links, Custom Message and the "Get My Location" button require internet access.</span>
    <span class="small">They won't work while the device is in AP Mode - connect to Wi-Fi first.</span>
  </div>

  <button type="button" class="collapsible-toggle" aria-expanded="false">
    <span class="icon-area" aria-hidden="true">
      <svg width="20" height="20" viewBox="0 0 16 16" version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
        <path  fill="currentColor" d="M16,9V7l-2.259-0.753c-0.113-0.372-0.262-0.728-0.441-1.066l1.066-2.131L12.95,1.634L10.819,2.7 c-0.338-0.178-0.694-0.325-1.066-0.441L9,0H7L6.247,2.259C5.875,2.372,5.519,2.522,5.181,2.7L3.05,1.638L1.638,3.05l1.066,2.131 C2.522,5.519,2.375,5.875,2.259,6.247L0,7v2l2.259,0.753c0.112,0.372,0.263,0.728,0.441,1.066L1.634,12.95l1.416,1.416L5.181,13.3 c0.338,0.178,0.694,0.328,1.066,0.441L7,16h2l0.753-2.259c0.372-0.113,0.728-0.262,1.066-0.441l2.131,1.066l1.416-1.416L13.3,10.819 c0.178-0.337,0.328-0.694,0.344-1.066L16,9z M8,11c-1.656,0-3-1.344-3-3s1.344-3,3-3s3,1.344,3,3S9.656,11,8,11z" />
      </svg>
    </span>
    <span>Advanced Settings</span>
  </button>

  <div class="collapsible-content" aria-hidden="true">
    <label>Primary NTP Server:</label>
    <input type="text" name="ntpServer1" id="ntpServer1" placeholder="Enter NTP address">

    <label>Secondary NTP Server:</label>
    <input type="text" name="ntpServer2" id="ntpServer2" placeholder="Enter NTP address">
    <div class="toggles" style="padding: 0 1rem;">

      <label style="display: flex; align-items: center; margin-top: 1.75rem; justify-content: space-between;">
        <span style="margin-right: 0.5em;">Show Day Of The Week:</span>
        <span class="toggle-switch">
          <input type="checkbox" id="showDayOfWeek" name="showDayOfWeek" onchange="setShowDayOfWeek(this.checked)">
          <span class="toggle-slider"></span>
        </span>
      </label>

      <label style="display: flex; align-items: center; margin-top: 1.75rem; justify-content: space-between;">
        <span style="margin-right: 0.5em;">Animated Seconds:</span>
        <span class="toggle-switch">
          <input type="checkbox" id="colonBlinkEnabled" name="colonBlinkEnabled" onchange="setColonBlink(this.checked)">
          <span class="toggle-slider"></span>
        </span>
      </label>

      <label style="display: flex; align-items: center; margin-top: 1.75rem; justify-content: space-between;">
        <span style="margin-right: 0.5em;">Show Date:</span>
        <span class="toggle-switch">
          <input type="checkbox" id="showDate" name="showDate" onchange="setShowDate(this.checked)">
          <span class="toggle-slider"></span>
        </span>
      </label>

      <label style="display: flex; align-items: center; margin-top: 1.75rem; justify-content: space-between;">
        <span style="margin-right: 0.5em;">Display 12-hour Clock:</span>
        <span class="toggle-switch">
          <input type="checkbox" id="twelveHourToggle" name="twelveHourToggle" onchange="setTwelveHour(this.checked)">
          <span class="toggle-slider"></span>
        </span>
      </label>

      <label style="display: flex; align-items: center; margin-top: 1.75rem; justify-content: space-between;">
        <span style="margin-right: 0.5em;">Use Imperial Units (°F):</span>
        <span class="toggle-switch">
          <input type="checkbox" id="weatherUnits" name="weatherUnits" onchange="setWeatherUnits(this.checked)">
          <span class="toggle-slider"></span>
        </span>
      </label>

      <label style="display: flex; align-items: center; margin-top: 1.75rem; justify-content: space-between;">
        <span style="margin-right: 0.5em;">Show Humidity:</span>
        <span class="toggle-switch">
          <input type="checkbox" id="showHumidity" name="showHumidity" onchange="setShowHumidity(this.checked)">
          <span class="toggle-slider"></span>
        </span>
      </label>

      <label style="display: flex; align-items: center; margin-top: 1.75rem; justify-content: space-between;">
        <span style="margin-right: 0.5em;">Show Weather Description:</span>
        <span class="toggle-switch">
          <input type="checkbox" id="showWeatherDescription" name="showWeatherDescription" onchange="setShowWeatherDescription(this.checked)">
          <span class="toggle-slider"></span>
        </span>
      </label>

      <label style="display: flex; align-items: center; margin-top: 1.75rem; justify-content: space-between;">
        <span style="margin-right: 0.5em;">Flip Display (180°):</span>
        <span class="toggle-switch">
          <input type="checkbox" id="flipDisplay" name="flipDisplay" onchange="setFlipDisplay(this.checked)">
          <span class="toggle-slider"></span>
        </span>
      </label>

      <label style="margin-top: 1.75rem;">Brightness: <span id="brightnessValue">10</span></label>
      <input style="width: 100%;" type="range" min="-1" max="15" name="brightness" id="brightnessSlider" value="10" 
      oninput="brightnessValue.textContent = (this.value == -1 ? 'Off' : this.value); setBrightnessLive(this.value);">
      <br><br><br>


<label style="display: flex; align-items: center; justify-content: space-between; margin-top: 1rem;">
  <span style="margin-right: 0.5em;">Automatic Dimming:</span>
  <span class="toggle-switch">
    <input type="checkbox" id="autoDimmingEnabled" name="autoDimmingEnabled">
    <span class="toggle-slider"></span>
  </span>
</label>
<div id="autoDimmingNote" class="small">
  Requires a valid OpenWeather API key.
</div>


<label style="display: flex; align-items: center; justify-content: space-between; margin-top: 0.75rem;">
  <span style="margin-right: 0.5em;">Custom Dimming:</span>
  <span class="toggle-switch">
    <input type="checkbox" id="dimmingEnabled" name="dimmingEnabled">
    <span class="toggle-slider"></span>
  </span>
</label>


<label label style="display: flex; align-items: center; justify-content: space-between; margin-top: 0.75rem;">
  <span style="margin-right: 0.5em;">Clock-Only Mode When Dimmed:</span>
  <span class="toggle-switch">
    <input type="checkbox" id="clockOnlyDuringDimming">
    <span class="toggle-slider"></span>
  </span>
</label>


<div class="form-row two-col">   
  <div>         
    <label for="dimStartTime">Start Time:</label>
    <input type="time" id="dimStartTime" value="18:00">
  </div>

  <div>
    <label for="dimEndTime">End Time:</label>
    <input type="time" id="dimEndTime" value="08:00">
  </div>
</div>

<label style="margin-top: 1.75rem;" for="dimBrightness">
  Dimming Brightness: <span id="dimmingBrightnessValue">2</span>
</label>
<input style="width: 100%;" type="range" min="-1" max="15"
  name="dimming_brightness" id="dimBrightness" value="2"
  oninput="dimmingBrightnessValue.textContent = (this.value == -1 ? 'Off' : this.value);">

      <br><br><br>
      <div class="form-group">
          <label style="display: flex; align-items: center; justify-content: space-between;">
              <span style="margin-right: 0.5em;">Enable Countdown:</span>
              <span class="toggle-switch">
                  <input type="checkbox" id="countdownEnabled" name="countdownEnabled">
                  <span class="toggle-slider"></span>
              </span>
          </label>
      </div>
      <div class="form-group">
        <label style="display: flex; align-items: center; justify-content: space-between;">
          <span style="margin-right: 0.5em;">Dramatic Countdown:</span>
          <span class="toggle-switch">
              <input type="checkbox" id="isDramaticCountdown" name="isDramaticCountdown">
              <span class="toggle-slider"></span>
          </span>
        </label>
      </div>
      <div class="form-row two-col">
        <div class="form-group">
            <label for="countdownDate">Date:</label>
            <input type="date" id="countdownDate" name="countdownDate" class="form-control">
        </div>
        <div class="form-group">
            <label for="countdownTime">Time:</label>
            <input type="time" id="countdownTime" name="countdownTime" class="form-control">
        </div>
      </div>
      <div class="form-group">
        <label for="countdownLabel">Countdown Label (Optional):</label>
        <input type="text" id="countdownLabel" name="countdownLabel" class="form-control"
          placeholder="e.g., BIRTHDAY, VACATION" maxlength="24"
          pattern="[A-Z0-9 :!'\-.,_\+%\/?]*"
          title="Only uppercase letters, numbers, space, and : ! ' - . , _ + % / ? allowed">
        <div class="small">Allowed characters: A–Z, 0–9, space, and : ! ' - . ? , _ + % /</div>
      </div>
    </div>   
  </div>
  <input type="submit" class="primary-button" value="Save Settings">
</form>

<div class="footer">
  Project by: <a href="https://www.instagram.com/mfactory.osaka" target="_blank" rel="noopener noreferrer">M-Factory</a><br>
  Device uptime: <span id="uptimeDisplay">Loading...</span>
</div>

<div id="savingMessage"></div>
<script>
let isSaving = false;
let isAPMode = false;

// Set initial value display for brightness
document.addEventListener('DOMContentLoaded', function() {
  brightnessValue.textContent = brightnessSlider.value;

 // Sanitize input LIVE for customMessage
  var customMsgInput = document.getElementById('customMessage');
  if (customMsgInput) {
    customMsgInput.addEventListener('input', function() {
      let before = this.value;
      let after = before.toUpperCase().replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, '');
      if (before !== after) {
        this.value = after;
      }
    });
  }

});

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

window.onload = function () {
  fetch('/config.json')
  .then(response => response.json())
  .then(data => {
    isAPMode = (data.mode === "ap");
    if (isAPMode) {
      document.querySelector('.geo-note').style.display = 'block';
      document.getElementById('geo-button').classList.add('geo-disabled');
      document.getElementById('geo-button').disabled = true;

      document.querySelector('.cmsg1').classList.add('geo-disabled');
      document.querySelector('.cmsg1').disabled = true;

      document.querySelector('.cmsg2').classList.add('geo-disabled');
      document.querySelector('.cmsg2').disabled = true;
    }
    document.getElementById('ssid').value = data.ssid || '';
    document.getElementById('password').value = data.password || '';
    const apiInput = document.getElementById('openWeatherApiKey');
    if (data.openWeatherApiKey && data.openWeatherApiKey.trim() !== '') {
      apiInput.value = MASK;
      hasSavedKey = true;
    } else {
      apiInput.value = '';
      hasSavedKey = false;
    }

    document.getElementById('openWeatherCity').value = data.openWeatherCity || '';
    document.getElementById('openWeatherCountry').value = data.openWeatherCountry || '';
    document.getElementById('weatherUnits').checked = (data.weatherUnits === "imperial");
    document.getElementById('clockDuration').value = (data.clockDuration || 10000) / 1000;
    document.getElementById('weatherDuration').value = (data.weatherDuration || 5000) / 1000;
    document.getElementById('language').value = data.language || '';

    // Advanced:
    document.getElementById('brightnessSlider').value = typeof data.brightness !== "undefined" ? data.brightness : 10;
    document.getElementById('brightnessValue').textContent = (document.getElementById('brightnessSlider').value == -1 ? 'Off' : document.getElementById('brightnessSlider').value);
    document.getElementById('flipDisplay').checked = !!data.flipDisplay;
    document.getElementById('ntpServer1').value = data.ntpServer1 || "";
    document.getElementById('ntpServer2').value = data.ntpServer2 || "";
    document.getElementById('twelveHourToggle').checked = !!data.twelveHourToggle;
    document.getElementById('showDayOfWeek').checked = !!data.showDayOfWeek;
    document.getElementById('showDate').checked = !!data.showDate;
    document.getElementById('showHumidity').checked = !!data.showHumidity;
    document.getElementById('colonBlinkEnabled').checked = !!data.colonBlinkEnabled;
    document.getElementById('showWeatherDescription').checked = !!data.showWeatherDescription;

    // --- Dimming Controls ---
    const autoDimmingEl = document.getElementById('autoDimmingEnabled');
    const dimmingEnabledEl = document.getElementById('dimmingEnabled');
    const apiInputEl = document.getElementById('openWeatherApiKey');

    // Evaluate flags from config.json
    const isAutoDimming = (data.autoDimmingEnabled === true || data.autoDimmingEnabled === "true" || data.autoDimmingEnabled === 1);
    const isCustomDimming = (data.dimmingEnabled === true || data.dimmingEnabled === "true" || data.dimmingEnabled === 1);

    // Set toggle states
    autoDimmingEl.checked = isAutoDimming;
    dimmingEnabledEl.checked = isCustomDimming;

    // Apply initial dimming state
    setDimmingFieldsEnabled();

    // Initialize Clock-only-during-dimming control (depends on dimming fields)
    initClockOnlyDuringDimming(data);

    // Attach listeners (mutually exclusive + API dependency)
    if (apiInputEl) apiInputEl.addEventListener('input', setDimmingFieldsEnabled);
    autoDimmingEl.addEventListener('change', () => {
      if (autoDimmingEl.checked) dimmingEnabledEl.checked = false;
      setDimmingFieldsEnabled();
      clearClockOnlyIfNoDimming();
    });
    dimmingEnabledEl.addEventListener('change', () => {
      if (dimmingEnabledEl.checked) autoDimmingEl.checked = false;
      setDimmingFieldsEnabled();
      clearClockOnlyIfNoDimming();
    });

    // Set field values from config
    document.getElementById('dimStartTime').value =
      (data.dimStartHour !== undefined ? String(data.dimStartHour).padStart(2, '0') : "18") + ":" +
      (data.dimStartMinute !== undefined ? String(data.dimStartMinute).padStart(2, '0') : "00");

    document.getElementById('dimEndTime').value =
      (data.dimEndHour !== undefined ? String(data.dimEndHour).padStart(2, '0') : "08") + ":" +
      (data.dimEndMinute !== undefined ? String(data.dimEndMinute).padStart(2, '0') : "00");

    document.getElementById('dimBrightness').value = (data.dimBrightness !== undefined ? data.dimBrightness : 2);
    document.getElementById('dimmingBrightnessValue').textContent =
      (document.getElementById('dimBrightness').value == -1 ? 'Off' : document.getElementById('dimBrightness').value);

    // --- Populate Countdown Fields ---
    document.getElementById('isDramaticCountdown').checked = !!(data.countdown && data.countdown.isDramaticCountdown);
    const countdownEnabledEl = document.getElementById('countdownEnabled'); // Get reference
    countdownEnabledEl.checked = !!(data.countdown && data.countdown.enabled);
    if (data.countdown && data.countdown.targetTimestamp) {
      const targetDate = new Date(data.countdown.targetTimestamp * 1000);
      const year = targetDate.getFullYear();
      const month = (targetDate.getMonth() + 1).toString().padStart(2, '0');
      const day = targetDate.getDate().toString().padStart(2, '0');
      const hours = targetDate.getHours().toString().padStart(2, '0');
      const minutes = targetDate.getMinutes().toString().padStart(2, '0');
      document.getElementById('countdownDate').value = `${year}-${month}-${day}`;
      document.getElementById('countdownTime').value = `${hours}:${minutes}`;
    } else {
      document.getElementById('countdownDate').value = '';
      document.getElementById('countdownTime').value = '';
    }

    // Countdown Label Input Validation 
    const countdownLabelInput = document.getElementById('countdownLabel');
    countdownLabelInput.addEventListener('input', function() {
      this.value = this.value.toUpperCase().replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, '');
    });
    if (data.countdown && data.countdown.label) {
      countdownLabelInput.value = data.countdown.label.toUpperCase().replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, '');
    } else {
      countdownLabelInput.value = '';
    }

    // Countdown Toggle Event Listener & Field Enabling 
    countdownEnabledEl.addEventListener('change', function() {
      setCountdownEnabled(this.checked);
      setCountdownFieldsEnabled(this.checked);
    });
    const dramaticCountdownEl = document.getElementById('isDramaticCountdown');
    dramaticCountdownEl.addEventListener('change', function () {
      setIsDramaticCountdown(this.checked);
    });
    setCountdownFieldsEnabled(countdownEnabledEl.checked);

    if (data.customMessage !== undefined) {
      document.getElementById('customMessage').value = data.customMessage;
    }

    // Auto-detect browser's timezone if not set in config
    if (!data.timeZone) {
      try {
        const tz = Intl.DateTimeFormat().resolvedOptions().timeZone;
        if (tz && document.getElementById('timeZone').querySelector(`[value="${tz}"]`)) {
          document.getElementById('timeZone').value = tz;
        } else {
          document.getElementById('timeZone').value = '';
        }
      } catch (e) {
        document.getElementById('timeZone').value = '';
      }
    } else {
      document.getElementById('timeZone').value = data.timeZone;
    }
  })
  .catch(err => {
    console.error('Failed to load config:', err);
    showSavingModal("");
    updateSavingModal("⚠️ Failed to load configuration.", false);
    // Show appropriate button for load error
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
  document.querySelector('html').style.height = 'unset';       
  document.body.classList.add('loaded');     
};

async function submitConfig(event) {
  event.preventDefault();
  isSaving = true;

  const form = document.getElementById('configForm');
  const formData = new FormData(form);

  const clockDuration = parseInt(formData.get('clockDuration')) * 1000;
  const weatherDuration = parseInt(formData.get('weatherDuration')) * 1000;
  formData.set('clockDuration', clockDuration);
  formData.set('weatherDuration', weatherDuration);

  let apiKeyToSend = apiInput.value;

  // If the user left the masked key untouched, skip sending it
  if (apiKeyToSend === MASK && hasSavedKey) {
    formData.delete('openWeatherApiKey');
  } else {
    formData.set('openWeatherApiKey', apiKeyToSend);
  }

  // Advanced: ensure correct values are set for advanced fields
  formData.set('brightness', document.getElementById('brightnessSlider').value);
  formData.set('flipDisplay', document.getElementById('flipDisplay').checked ? 'on' : '');
  formData.set('twelveHourToggle', document.getElementById('twelveHourToggle').checked ? 'on' : '');
  formData.set('showDayOfWeek', document.getElementById('showDayOfWeek').checked ? 'on' : '');
  formData.set('showDate', document.getElementById('showDate').checked ? 'on' : '');
  formData.set('showHumidity', document.getElementById('showHumidity').checked ? 'on' : '');
  formData.set('colonBlinkEnabled', document.getElementById('colonBlinkEnabled').checked ? 'on' : '');

  // --- Dimming ---
  const autoDimmingChecked = document.getElementById('autoDimmingEnabled').checked;
  const customDimmingChecked = document.getElementById('dimmingEnabled').checked;

  // Mutual exclusivity (if both checked somehow, keep auto as priority)
  if (autoDimmingChecked && customDimmingChecked) {
    formData.set('autoDimmingEnabled', 'true');
    formData.set('dimmingEnabled', 'false');
  } else {
    formData.set('autoDimmingEnabled', autoDimmingChecked ? 'true' : 'false');
    formData.set('dimmingEnabled', customDimmingChecked ? 'true' : 'false');
  }

  const dimStart = document.getElementById('dimStartTime').value; // "18:45"
  const dimEnd = document.getElementById('dimEndTime').value; // "08:30"

  // Parse hour and minute
  if (dimStart) {
    const [startHour, startMin] = dimStart.split(":").map(x => parseInt(x, 10));
    formData.set('dimStartHour', startHour);
    formData.set('dimStartMinute', startMin);
  }
  if (dimEnd) {
    const [endHour, endMin] = dimEnd.split(":").map(x => parseInt(x, 10));
    formData.set('dimEndHour', endHour);
    formData.set('dimEndMinute', endMin);
  }
  formData.set('dimBrightness', document.getElementById('dimBrightness').value);
  formData.set('showWeatherDescription', document.getElementById('showWeatherDescription').checked ? 'on' : '');
  formData.set('weatherUnits', document.getElementById('weatherUnits').checked ? 'imperial' : 'metric');

  // --- NEW: Countdown Form Data ---
  formData.set('countdownEnabled', document.getElementById('countdownEnabled').checked ? 'true' : 'false');
  formData.set('isDramaticCountdown', document.getElementById('isDramaticCountdown').checked ? 'true' : 'false');
  // Date and Time inputs are already handled by formData if they have a 'name' attribute
  // 'countdownDate' and 'countdownTime' are collected automatically
  // Also apply the same validation for the label when submitting
  const finalCountdownLabel = document.getElementById('countdownLabel').value.toUpperCase().replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, '');
  formData.set('countdownLabel', finalCountdownLabel);
  // --- END NEW ---

  const params = new URLSearchParams();
  for (const pair of formData.entries()) {
    params.append(pair[0], pair[1]);
  }

  // Sanitize and set customMessage before sending
  const customMsgInput = document.getElementById('customMessage');
  if (customMsgInput) {
    customMsgInput.value = customMsgInput.value
      .toUpperCase()
      .replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, '')
      .replace(/\s+/g, ' ')
      .trim()
      .substring(0, 120);
  }

  // Check AP mode status
  let isAPMode = false;
  try {
    const apStatusResponse = await fetch('/ap_status');
    const apStatusData = await apStatusResponse.json();
    isAPMode = apStatusData.isAP;
  } catch (error) {
    console.error("Error fetching AP status:", error);
    // Handle error appropriately (e.g., assume not in AP mode)
  }

  if (isAPMode) {
    showSavingModal("");
    updateSavingModal(
      "✅ Settings saved successfully!<br><br>" +
      "Rebooting the device now...<br><br>" +
      "Your device will connect to your Wi-Fi.<br>" +
      "Its new IP address will appear on the display for future access.",
      true // show spinner
    );
    } else{
      showSavingModal("");
    };  

  await new Promise(resolve => setTimeout(resolve, isAPMode ? 5000 : 0));
  fetch('/save', {
    method: 'POST',
    body: params
  })
  .then(response => {
    if (!response.ok) {
      return response.json().then(json => {
        throw new Error(`Server error ${response.status}: ${json.error}`);
      });
    }
    return response.json();
  })
  .then(json => {
    isSaving = false;
    removeReloadButton();
    removeRestoreButton();

    if (isAPMode) {
      setTimeout(() => {
      document.getElementById('configForm').style.display = 'none';
      document.querySelector('.footer').style.display = 'none';
      document.querySelector('html').style.height = '100vh';
      document.body.style.height = '100vh';
      updateSavingModal(
        "✅ All done!<br>You can now close this tab safely.<br><br>" +
        "Your device has rebooted and is now connected to your Wi-Fi.<br>" +
        "Check the display for the current IP address.",
        false // stop spinner
      );
    }, 5000);
  return;
  } else {
      showSavingModal("");
      updateSavingModal("✅ Configuration saved successfully.<br><br>Device will reboot", false);
      setTimeout(() => location.href = location.href.split('#')[0], 3000);
    }
  })
  .catch(err => {
    isSaving = false;

    if (isAPMode && err.message.includes("Failed to fetch")) {
      console.warn("Expected disconnect in AP mode after reboot.");
      showSavingModal("");
      updateSavingModal("✅ Settings saved successfully!<br><br>Rebooting the device now... ", false);
      setTimeout(() => {
        document.getElementById('configForm').style.display = 'none'; 
        updateSavingModal("✅ All done!<br>You can now close this tab safely.<br><br>" +
        "Your device has rebooted and is now connected to your Wi-Fi.<br>" +
        "Check the display for the current IP address.", false);
      }, 5000);
      removeReloadButton();
      removeRestoreButton();
      return;
    }

    console.error('Save error:', err);
    let friendlyMessage = "⚠️ Something went wrong while saving the configuration.";
    if (err.message.includes("Failed to fetch")) {
      friendlyMessage = "⚠️ Cannot connect to the device.<br>Is it powered on and connected?";
    }

    updateSavingModal(`${friendlyMessage}<br><br>Details: ${err.message}`, false);

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
  let modal = document.getElementById('savingModal');
  if (!modal) {
    modal = document.createElement('div');
    modal.id = 'savingModal';
    modal.innerHTML = `
      <div id="savingModalContent">
        <div class="spinner"></div>
        <div id="savingModalText">${message}</div>
      </div>
    `;
    document.body.appendChild(modal);
  } else {
    document.getElementById('savingModalText').innerHTML = message;
    document.querySelector('#savingModal .spinner').style.display = 'block';
  }
  modal.style.display = 'flex';
  document.body.classList.add('modal-open');
}

function updateSavingModal(message, showSpinner = false) {
  let modalText = document.getElementById('savingModalText');
  modalText.innerHTML = message;
  document.querySelector('#savingModal .spinner').style.display = showSpinner ? 'block' : 'none';

  // Remove reload/restore buttons if no longer needed
  if (message.includes("saved successfully") || message.includes("Backup restored") || message.includes("All done!")) {
    removeReloadButton();
    removeRestoreButton();
  }
}

function ensureReloadButton(options = {}) {
  let modalContent = document.getElementById('savingModalContent');
  if (!modalContent) return;
  let btn = document.getElementById('reloadButton');
  if (!btn) {
    btn = document.createElement('button');
    btn.id = 'reloadButton';
    btn.className = 'primary-button';
    btn.style.display = 'inline-block';
    btn.style.margin = '1rem 0.5rem 0 0';
    modalContent.appendChild(btn);
  }
  btn.textContent = options.text || "Reload Page";
  btn.onclick = options.onClick || (() => location.reload());
  btn.style.display = 'inline-block';
  return btn;
}

function ensureRestoreButton(options = {}) {
  let modalContent = document.getElementById('savingModalContent');
  if (!modalContent) return;
  let btn = document.getElementById('restoreButton');
  if (!btn) {
    btn = document.createElement('button');
    btn.id = 'restoreButton';
    btn.className = 'primary-button';
    btn.style.display = 'inline-block';
    btn.style.margin = '1rem 0 0 0.5rem';
    modalContent.appendChild(btn);
  }
  btn.textContent = options.text || "Restore Backup";
  btn.onclick = options.onClick || restoreBackupConfig;
  btn.style.display = 'inline-block';
  return btn;
}

function removeReloadButton() {
  let btn = document.getElementById('reloadButton');
  if (btn && btn.parentNode) btn.parentNode.removeChild(btn);
}
function removeRestoreButton() {
  let btn = document.getElementById('restoreButton');
  if (btn && btn.parentNode) btn.parentNode.removeChild(btn);
}
function restoreBackupConfig() {
  showSavingModal("Restoring backup...");
  removeReloadButton();
  removeRestoreButton();

  fetch('/restore', { method: 'POST' })
  .then(response => {
    if (!response.ok) {
      throw new Error("Server returned an error");
    }
    return response.json();
  })
  .then(data => {
    updateSavingModal("✅ Backup restored! Device will now reboot.");
    setTimeout(() => location.reload(), 1500);
  })
  .catch(err => {
    console.error("Restore error:", err);
    updateSavingModal(`❌ Failed to restore backup: ${err.message}`, false);

    // Show only one button, for backup restore failures show reload.
    removeReloadButton();
    removeRestoreButton();
    ensureReloadButton();
  });
}

function hideSavingModal() {
  const modal = document.getElementById('savingModal');
  if (modal) {
    modal.style.display = 'none';
    document.body.classList.remove('modal-open');
  }
}

const toggle = document.querySelector('.collapsible-toggle');
const content = document.querySelector('.collapsible-content');
toggle.addEventListener('click', function() {
  const isOpen = toggle.classList.toggle('open');
  toggle.setAttribute('aria-expanded', isOpen);
  content.setAttribute('aria-hidden', !isOpen);
  if(isOpen) {
    content.style.height = content.scrollHeight + 'px';
    content.addEventListener('transitionend', function handler() {
      content.style.height = 'auto';
      content.removeEventListener('transitionend', handler);
    });
  } else {
    content.style.height = content.scrollHeight + 'px';
    // Force reflow to make sure the browser notices the height before transitioning to 0
    void content.offsetHeight;
    content.style.height = '0px';
  }
});
// Optional: If open on load, set height to auto
if(toggle.classList.contains('open')) {
  content.style.height = 'auto';
}

let brightnessDebounceTimeout = null;

function setBrightnessLive(val) {
  // Cancel the previous timeout if it exists
  if (brightnessDebounceTimeout) {
    clearTimeout(brightnessDebounceTimeout);
  }

  // Set a new timeout
  brightnessDebounceTimeout = setTimeout(() => {
    fetch('/set_brightness', {
      method: 'POST',
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
        "X-Source": "UI"
      },
      body: "value=" + encodeURIComponent(val)
    })
    .then(res => res.json())
    .catch(e => {}); // Optionally handle errors
  }, 150); // 150ms debounce
}

function setFlipDisplay(val) {
  fetch('/set_flip', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  });
}

function setTwelveHour(val) {
  fetch('/set_twelvehour', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  });
}

function setShowDayOfWeek(val) {
  fetch('/set_dayofweek', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  });
}

function setShowDate(val) {
  fetch('/set_showdate', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  });
}

function setColonBlink(val) {
  fetch('/set_colon_blink', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  });
}

function setShowHumidity(val) {
  fetch('/set_humidity', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  });
}

function setLanguage(val) {
  fetch('/set_language', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + encodeURIComponent(val)
  });
}

function setShowWeatherDescription(val) {
  fetch('/set_weatherdesc', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  });
}

function setWeatherUnits(val) {
  fetch('/set_units', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  });
}

// --- Clock-only-during-dimming setter (no reboot) ---
function setClockOnlyDuringDimming(val) {
  fetch('/set_clock_only_dimming', {
    method: 'POST',
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: "value=" + (val ? 1 : 0)
  }).catch(e => {
    console.error('Failed to set clockOnlyDuringDimming:', e);
  });
}

// Initialize the checkbox from cfg and wire up immediate save (no reboot)
function initClockOnlyDuringDimming(cfg) {
  const el = document.getElementById('clockOnlyDuringDimming');
  if (!el) return;

  // Set initial checked value from config
  el.checked = !!cfg.clockOnlyDuringDimming;

  // Determine whether dimming is enabled (auto OR custom)
  const autoDim = (cfg.autoDimmingEnabled === true || cfg.autoDimmingEnabled === "true" || cfg.autoDimmingEnabled === 1);
  const manualDim = (cfg.dimmingEnabled === true || cfg.dimmingEnabled === "true" || cfg.dimmingEnabled === 1);

  // Normalize dimBrightness from config (handle "Off" or "-1" string)
  let db = cfg.dimBrightness;
  if (typeof db === 'string') {
    if (db.toLowerCase() === 'off') db = -1;
    else db = parseInt(db, 10);
  }
  const dimBrightnessOk = (typeof db === 'number') ? (db !== -1) : true;

  // Enable only when some dimming mode is active and dimming does not fully turn display off
  el.disabled = !(autoDim || manualDim) || !dimBrightnessOk;

  // On change, persist immediately (no reboot)
  el.addEventListener('change', function () {
    const want = this.checked;
    setClockOnlyDuringDimming(want);
    // optimistic UI: leave checkbox as toggled; if server fails we don't roll back here
  });
}

// --- Countdown Controls Logic ---
// NEW: Function to enable/disable countdown specific fields
function setCountdownFieldsEnabled(enabled) {
  document.getElementById('countdownLabel').disabled = !enabled;
  document.getElementById('countdownDate').disabled = !enabled;
  document.getElementById('countdownTime').disabled = !enabled;
  document.getElementById('isDramaticCountdown').disabled = !enabled;
}

// Existing function to send countdown enable/disable command to ESP
function setCountdownEnabled(val) {
  fetch('/set_countdown_enabled', {
      method: 'POST',
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: "value=" + (val ? 1 : 0) // Send 1 for true, 0 for false
  });
}

function setIsDramaticCountdown(val) {
  fetch('/set_dramatic_countdown', {
      method: 'POST',
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: "value=" + (val ? 1 : 0) // Send 1 for true, 0 for false
  });
}
// --- END Countdown Controls Logic ---


// --- Dimming Controls Logic ---
function setDimmingFieldsEnabled(enabled) {
  document.getElementById('dimStartTime').disabled = !enabled;
  document.getElementById('dimEndTime').disabled = !enabled;
  document.getElementById('dimBrightness').disabled = !enabled;
}



async function getLocation() {
  const normalize = v => {
    if (v === null || v === undefined) return '';
    const s = String(v).trim();
    if (!s || s.toLowerCase() === 'null' || s.toLowerCase() === 'none' || s === '-') return '';
    return s;
  };

  const setFields = (lat, lon, label) => {
    if (lat) document.getElementById('openWeatherCity').value = lat;
    if (lon) document.getElementById('openWeatherCountry').value = lon;
    const btn = document.getElementById('geo-button');
    btn.textContent = "Location: " + (label || "Location Found");
    btn.disabled = true;
    btn.classList.add('geo-disabled');
  };

  try {
    // 1) get your public IP
    const ipResp = await fetch('https://api.ipify.org?format=json');
    if (!ipResp.ok) throw new Error('ipify failed: ' + ipResp.status);
    const { ip } = await ipResp.json();
    if (!ip) throw new Error('no IP returned by ipify');

    // 2) call HackerTarget GeoIP with JSON output
    const geoResp = await fetch(`https://api.hackertarget.com/geoip/?q=${encodeURIComponent(ip)}&output=json`);
    if (!geoResp.ok) throw new Error('HackerTarget returned ' + geoResp.status);
    const data = await geoResp.json();

    // 3) extract and normalize fields
    const lat = data.latitude;
    const lon = data.longitude;
    const city = normalize(data.city);
    const state = normalize(data.state);
    const country = normalize(data.country);
    const label = city || state || country || "Location Found";

    if (!lat || !lon) throw new Error('missing latitude/longitude');

    setFields(lat, lon, label);
    //console.log('Location fetched via HackerTarget. Label:', label);
  } catch (err) {
    console.error('HackerTarget geolocation failed:', err);
    alert(
      "Failed to guess your location using HackerTarget.\n\n" +
      "Possible causes:\n" +
      "- CORS blocking in browser (try server-side)\n" +
      "- Network issue or rate limit\n\n" +
      "You can manually search for coordinates on https://openweathermap.org/find"
    );
  }
}

// --- OpenWeather API Key field UX ---
const MASK_LENGTH = 32;
const MASK = '*'.repeat(MASK_LENGTH);
const apiInput = document.getElementById('openWeatherApiKey');
let hasSavedKey = false;

// --- Initialize the field after config load ---
if (apiInput.value && apiInput.value.trim() !== '') {
  apiInput.value = MASK;   // show mask
  hasSavedKey = true;
} else {
  apiInput.value = '';
  hasSavedKey = false;
}

// --- Detect user clearing intent ---
apiInput.addEventListener('input', () => {
  apiInput.dataset.clearing = apiInput.value === '' ? 'true' : 'false';
});

// --- Handle Delete/Backspace when focused but empty ---
apiInput.addEventListener('keydown', (e) => {
  if ((e.key === 'Backspace' || e.key === 'Delete') && apiInput.value === '') {
    apiInput.dataset.clearing = 'true';
  }
});

// --- Focus handler: clear mask for editing ---
apiInput.addEventListener('focus', () => {
  if (apiInput.value === MASK) apiInput.value = '';
});

// --- Blur handler: restore mask if user didn’t clear the field ---
apiInput.addEventListener('blur', () => {
  if (apiInput.value === '') {
    if (hasSavedKey && apiInput.dataset.clearing !== 'true') {
      apiInput.value = MASK; // remask
    } else {
      hasSavedKey = false; // user cleared the key
      apiInput.dataset.clearing = 'false';
      apiInput.value = ''; // leave blank
      setDimmingFieldsEnabled();
    }
  }
});

// --- Uptime Tracker ---

let uptimeSeconds = 0;
let uptimeTimer;

// Fetch uptime from ESP
function fetchUptime() {
  fetch('/uptime')
    .then(res => res.text())
    .then(text => {
      uptimeSeconds = parseUptimeToSeconds(text);
      updateUptimeDisplay();
      if (uptimeTimer) clearInterval(uptimeTimer);
      uptimeTimer = setInterval(() => {
        uptimeSeconds++;
        updateUptimeDisplay();
      }, 1000);
    })
    .catch(err => console.error('Error fetching /uptime:', err));
}

// Convert "14:56:54" or "2d 03:12:33" → total seconds
function parseUptimeToSeconds(text) {
  let days = 0, h = 0, m = 0, s = 0;
  const dayMatch = text.match(/(\d+)\s*(?:d|day)/i);
  if (dayMatch) days = parseInt(dayMatch[1]);
  const timeMatch = text.match(/(\d+):(\d+):(\d+)/);
  if (timeMatch) {
    h = parseInt(timeMatch[1]);
    m = parseInt(timeMatch[2]);
    s = parseInt(timeMatch[3]);
  }
  return days * 86400 + h * 3600 + m * 60 + s;
}

// Format seconds → "2 days 04:09:31", "1 day 03:05:12", or "03:05:12"
function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  seconds %= 86400;
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;

  const timePart = `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(s).padStart(2,'0')}`;
  if (days > 1) return `${days} days ${timePart}`;
  if (days === 1) return `1 day ${timePart}`;
  return timePart;
}

// Update the text on screen
function updateUptimeDisplay() {
  document.getElementById('uptimeDisplay').textContent = formatUptime(uptimeSeconds);
}

// Start it up
fetchUptime();


function sendCustomMessage() {
  const input = document.getElementById('customMessage');
  let message = input.value
    .toUpperCase()
    .replace(/[^A-Z0-9 :!'\-.,_\+%\/?]/g, '')
    .replace(/\s+/g, ' ')
    .trim()
    .substring(0, 120);

  fetch('/set_custom_message', {
    method: 'POST',
    headers: { 
      "Content-Type": "application/x-www-form-urlencoded",
      "X-Source": "UI"
    },
    body: "message=" + encodeURIComponent(message)
  })
  .then(res => res.text())
  .then(res => {
    showSavingModal("");
    updateSavingModal("✅ Message sent successfully!<br><br>Now displaying your custom message.", false);
    setTimeout(hideSavingModal, 2000);
  })
  .catch(err => {
    console.error("Error sending custom message:", err);
    showSavingModal("");
    updateSavingModal("⚠️ Failed to send message.<br><br>Check connection.", false);
    setTimeout(hideSavingModal, 3000);
  });
}

function clearCustomMessage() {
  fetch('/set_custom_message', {
    method: 'POST',
    headers: { 
      "Content-Type": "application/x-www-form-urlencoded",
      "X-Source": "UI"   
    },
    body: "message="
  })
  .then(res => res.text())
  .then(res => {
    document.getElementById('customMessage').value = '';
    showSavingModal("");
    updateSavingModal("✅ Custom message cleared.<br><br>Display reverted to normal.", false);
    setTimeout(hideSavingModal, 2000);
  })
  .catch(err => {
    console.error("Error clearing custom message:", err);
    showSavingModal("");
    updateSavingModal("⚠️ Failed to clear message.<br><br>Check connection.", false);
    setTimeout(hideSavingModal, 3000);
  });
}

// --- Dimming Controls Logic (The correct version) ---
function setDimmingFieldsEnabled() {
  const apiKeyField = document.getElementById('openWeatherApiKey');
  const autoDimming = document.getElementById('autoDimmingEnabled');
  const dimmingEnabled = document.getElementById('dimmingEnabled');
  const dimStart = document.getElementById('dimStartTime'); 
  const dimEnd = document.getElementById('dimEndTime');    
  const dimBrightness = document.getElementById('dimBrightness');
  const noteEl = document.getElementById('autoDimmingNote');

  if (!apiKeyField || !autoDimming || !dimmingEnabled) return;

  const currentApiKeyInput = apiKeyField.value.trim();
  // Checks if a key is saved (hasSavedKey) OR if the user is currently typing a new one.
  const isKeyPresent = hasSavedKey || (currentApiKeyInput !== '' && currentApiKeyInput !== MASK);

  // --- 1. Control Auto Dimming based on Key Presence ---
  // Meets requirement: "when page load after autodim has been saved to json, 
  // if user removes the api key (masked) the toggle auto dim toggle should get disabled"
  if (!isKeyPresent) {
    autoDimming.checked = false; 
    autoDimming.disabled = true;
    if (noteEl) noteEl.style.display = 'block';
  } else {
    autoDimming.disabled = false;
    if (noteEl) noteEl.style.display = 'none';
  }
  
  // Custom Dimming toggle is always enabled (since it's not key-dependent)
  dimmingEnabled.disabled = false;


  // --- 2. Control Dependent Fields based on Active Mode ---

  const isAutoDimmingActive = autoDimming.checked && isKeyPresent; // Auto is only active if checked AND key is present
  const isCustomDimmingActive = dimmingEnabled.checked;
  const isDimmingActive = isAutoDimmingActive || isCustomDimmingActive; // Brightness slider logic

    // --- Update Clock-only-during-dimming checkbox state (if present) ---
  const clockOnlyEl = document.getElementById('clockOnlyDuringDimming');
  if (clockOnlyEl) {
    // Read current brightness control value (string)
    const dbEl = document.getElementById('dimBrightness');
    const dbVal = dbEl ? dbEl.value : null;
    const dbOk = (dbVal !== null) ? !(String(dbVal).toLowerCase() === 'off' || String(dbVal) === '-1') : true;
    const currentlyDimEnabled = isAutoDimmingActive || isCustomDimmingActive;
    clockOnlyEl.disabled = !currentlyDimEnabled || !dbOk;
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
  const autoEl = document.getElementById('autoDimmingEnabled');
  const dimEl = document.getElementById('dimmingEnabled');
  const clockEl = document.getElementById('clockOnlyDuringDimming');
  if (!autoEl || !dimEl || !clockEl) return;

  if (!autoEl.checked && !dimEl.checked) {
    // if currently checked, uncheck and persist change immediately
    if (clockEl.checked) {
      clockEl.checked = false;
      // persist without reboot
      fetch('/set_clock_only_dimming', {
        method: 'POST',
        body: new URLSearchParams({ value: 'false' })
      }).catch(e => console.error('Failed to persist clockOnlyDuringDimming clear:', e));
    }
    // also ensure it's disabled in the UI
    clockEl.disabled = true;
  }
}

window.addEventListener('DOMContentLoaded', () => {
  const apiKeyEl = document.getElementById('openWeatherApiKey');
  const autoEl = document.getElementById('autoDimmingEnabled');
  const dimEl = document.getElementById('dimmingEnabled');

  if (apiKeyEl) {
    apiKeyEl.addEventListener('input', setDimmingFieldsEnabled);
    apiKeyEl.addEventListener('change', setDimmingFieldsEnabled);
  }
  if (autoEl) autoEl.addEventListener('change', setDimmingFieldsEnabled);
  if (dimEl) dimEl.addEventListener('change', setDimmingFieldsEnabled);
});

</script>
</body>
</html>
)rawliteral";
