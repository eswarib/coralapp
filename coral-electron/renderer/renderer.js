const fs = require('fs');
const path = require('path');
const os = require('os');
const { ipcRenderer } = require('electron');

function getAppImageMountPath() {
  if (process.env.APPIMAGE) {
    return path.join(path.dirname(process.execPath), '..', '..', '..');
  }
  return null;
}

// Resolve config.json path depending on environment (AppImage vs dev)
let configPath;
if (process.env.APPIMAGE) {
  const appImageMountPath = getAppImageMountPath();
  const defaultConfigPath = path.join(appImageMountPath, 'usr', 'share', 'coral', 'conf', 'config.json');
  const userConfigDir = path.join(os.homedir(), '.coral');
  const userConfigPath = path.join(userConfigDir, 'conf', 'config.json');
  try {
    if (!fs.existsSync(userConfigDir)) {
      fs.mkdirSync(userConfigDir, { recursive: true });
    }
    if (!fs.existsSync(userConfigPath) && fs.existsSync(defaultConfigPath)) {
      fs.copyFileSync(defaultConfigPath, userConfigPath);
    }
  } catch (e) {
    console.error('Failed to prepare user config:', e.message);
  }
  configPath = userConfigPath;
} else {
  // Development: always use ~/.coral/conf/config.json; copy from platform config on first run
  const userConfigDir = path.join(os.homedir(), '.coral');
  configPath = path.join(userConfigDir, 'conf', 'config.json');
  const platformConfig = process.platform === 'win32' ? 'config.json' : 'config-linux.json';
  const devDefault = path.join(__dirname, '../..', 'coral', 'conf', platformConfig);
  try {
    if (!fs.existsSync(path.dirname(configPath))) fs.mkdirSync(path.dirname(configPath), { recursive: true });
    if (!fs.existsSync(configPath) && fs.existsSync(devDefault)) {
      fs.copyFileSync(devDefault, configPath);
    }
  } catch (e) { console.error('Failed to prepare user config:', e.message); }
}
const configForm = document.getElementById('configForm');
const saveBtn = document.getElementById('saveBtn');
const defaultBtn = document.getElementById('defaultBtn');
const statusDiv = document.getElementById('status');

// alert("renderer.js started");
document.getElementById('status').textContent = 'renderer.js loaded';
// alert("Before calling loadConfig()");
// alert("About to call loadConfig()");
// alert("typeof loadConfig: " + typeof loadConfig);
loadConfig();

let config = {};

function formatKeyComboFromEvent(e) {
  const parts = [];
  if (e.ctrlKey) parts.push('ctrl');
  if (e.altKey) parts.push('alt');
  if (e.shiftKey) parts.push('shift');
  if (e.metaKey) parts.push('meta');

  let key = '';
  if (e.code && e.code.startsWith('Key')) {
    key = e.code.slice(3).toLowerCase();
  } else if (e.code && e.code.startsWith('Digit')) {
    key = e.code.slice(5);
  } else if (e.key && e.key.length === 1) {
    key = e.key.toLowerCase();
  } else if (e.key) {
    key = e.key.toLowerCase(); // e.g., 'enter', 'escape', 'space'
  }
  // Avoid pure modifier shortcuts
  const isOnlyModifier = !key || ['control','ctrl','alt','shift','meta','super'].includes(key);
  if (!isOnlyModifier) parts.push(key);
  return parts.join('+');
}

function renderMainForm(cfg) {
  configForm.innerHTML = '';
  const primaryKeys = new Set(['triggerKey', 'cmdTriggerKey', 'whisperModelPath']);

  const renderField = (key, value) => {
    const label = document.createElement('label');
    label.textContent = key;
    let input;
    if (key === 'whisperModelPath') {
      // Custom UI: toolbar-like label + pill selector
      label.textContent = 'Model';
      label.className = 'toolbar-label';
      const hidden = document.createElement('input');
      hidden.type = 'hidden';
      hidden.name = key;
      hidden.value = value;
      // Derive current model display
      const displayForPath = (p) => {
        const v = (p || '').toLowerCase();
        if (v.includes('ggml-base.en.bin')) return 'Whisper base';
        if (v.includes('ggml-small.en-q8_0.bin') || v.includes('ggml-small.en.bin')) return 'Whisper small';
        return 'Custom whisper';
      };
      let currentLabel = displayForPath(value);
      const dropdown = document.createElement('div');
      dropdown.className = 'dropdown';
      const pill = document.createElement('span');
      pill.className = 'pill-select';
      pill.textContent = currentLabel;
      const menu = document.createElement('div');
      menu.className = 'dropdown-menu';
      menu.style.display = 'none';
      const items = ['Whisper base', 'Whisper small', 'Custom whisper'];
      items.forEach(txt => {
        const it = document.createElement('div');
        it.className = 'dropdown-item';
        it.textContent = txt;
        it.onclick = async () => {
          pill.textContent = txt;
          menu.style.display = 'none';
          if (txt === 'Whisper base') {
            hidden.value = 'ggml-base.en.bin';
          } else if (txt === 'Whisper small') {
            // Prefer q8 if available in search paths; backend will resolve
            hidden.value = 'ggml-small.en-q8_0.bin';
          } else {
            // Custom: open file picker and set absolute path
            const fp = await ipcRenderer.invoke('select-model-file');
            if (fp) {
              hidden.value = fp;
            }
          }
          requestResize();
        };
        menu.appendChild(it);
      });
      pill.onclick = () => {
        menu.style.display = (menu.style.display === 'none') ? 'block' : 'none';
        requestResize();
      };
      dropdown.appendChild(pill);
      dropdown.appendChild(menu);
      label.appendChild(dropdown);
      // subtle hint of current mapping
      const hint = document.createElement('span');
      hint.className = 'muted';
      hint.textContent = `(current: ${currentLabel})`;
      label.appendChild(hint);
      label.appendChild(hidden);
    } else if (key === 'cmdTriggerKey') {
      input = document.createElement('input');
      input.type = 'text';
      input.readOnly = true;
      input.placeholder = 'Click, then press shortcut (e.g., ctrl+shift+k)';
      input.value = value || '';
      input.name = key;
      input.addEventListener('keydown', (e) => {
        e.preventDefault();
        e.stopPropagation();
        const combo = formatKeyComboFromEvent(e);
        if (combo) input.value = combo;
      });
      input.addEventListener('focus', () => { statusDiv.textContent = 'Press desired shortcut…'; });
      input.addEventListener('blur', () => { statusDiv.textContent = ''; });
      label.appendChild(input);
    } else if (typeof value === 'boolean') {
      input = document.createElement('select');
      ['true', 'false'].forEach(opt => {
        const option = document.createElement('option');
        option.value = opt;
        option.text = opt;
        if (String(value) === opt) option.selected = true;
        input.appendChild(option);
      });
      input.name = key;
      label.appendChild(input);
    } else if (typeof value === 'number') {
      input = document.createElement('input');
      input.type = 'number';
      input.value = value;
      input.name = key;
      label.appendChild(input);
    } else {
      input = document.createElement('input');
      input.type = 'text';
      input.value = value;
      input.name = key;
      label.appendChild(input);
    }
    configForm.appendChild(label);
  };

  for (const [key, value] of Object.entries(cfg)) {
    if (primaryKeys.has(key)) {
      renderField(key, value);
    }
  }
  requestResize();
}

function requestResize() {
  try {
    const formBottom = configForm.getBoundingClientRect().bottom;
    const desired = Math.ceil(window.scrollY + formBottom + 24);
    ipcRenderer.send('resize-window', desired);
    // Recheck shortly after layout/paint to catch late style recalcs
    setTimeout(() => {
      const formBottom2 = configForm.getBoundingClientRect().bottom;
      const desired2 = Math.ceil(window.scrollY + formBottom2 + 24);
      ipcRenderer.send('resize-window', desired2);
    }, 50);
  } catch (_) {}
}

window.addEventListener('load', () => {
  requestResize();
});

function loadConfig() {
  // alert("Inside loadConfig()");
  // alert("configPath: " + configPath);
  // alert("fs: " + typeof fs);
  // alert("statusDiv: " + statusDiv);
  // alert("Trying to load config from: " + configPath); // Debug alert
  // console.log("Trying to load config from:", configPath); // Debug log
  fs.readFile(configPath, 'utf8', (err, data) => {
    if (err) {
      statusDiv.textContent = 'Failed to load config: ' + err.message;
      statusDiv.style.color = 'red';
      statusDiv.style.fontSize = '18px';
      statusDiv.style.margin = '10px';
      // alert('Failed to load config: ' + err.message); // Debug alert
      // console.log('Failed to load config:', err.message); // Debug log
      return;
    }
    try {
      config = JSON.parse(data);
      renderMainForm(config);
      statusDiv.textContent = '';
      statusDiv.style.color = '';
      statusDiv.style.fontSize = '';
      statusDiv.style.margin = '';
      // console.log('Config loaded:', config); // Debug log
    } catch (e) {
      statusDiv.textContent = 'Invalid config.json: ' + e.message;
      statusDiv.style.color = 'red';
      statusDiv.style.fontSize = '18px';
      statusDiv.style.margin = '10px';
      // alert('Invalid config.json: ' + e.message); // Debug alert
      // console.log('Invalid config.json:', e.message); // Debug log
    }
  });
}

saveBtn.onclick = (e) => {
  e.preventDefault();
  const newConfig = { ...config };
  for (const el of configForm.elements) {
    if (!el.name) continue;
    if (el.tagName === 'SELECT') {
      newConfig[el.name] = el.value === 'true';
    } else if (el.type === 'number') {
      newConfig[el.name] = Number(el.value);
    } else {
      newConfig[el.name] = el.value;
    }
  }
  // Validate model selection: allow known tokens or a valid file
  try {
    const modelPath = newConfig['whisperModelPath'];
    const tokens = ['ggml-base.en.bin', 'ggml-small.en.bin', 'ggml-small.en-q8_0.bin'];
    const isToken = tokens.includes(modelPath);
    if (!isToken) {
      if (!modelPath || !fs.existsSync(modelPath) || !fs.statSync(modelPath).isFile()) {
        statusDiv.textContent = 'Please select a valid model (choose preset or browse a file).';
        statusDiv.style.color = 'red';
        return;
      }
    }
  } catch (_) {}
  // Validate that whisperModelPath is a file (not a folder)
  try {
    const modelPath = newConfig['whisperModelPath'];
    if (!modelPath || !fs.existsSync(modelPath) || !fs.statSync(modelPath).isFile()) {
      statusDiv.textContent = 'Please select a valid model file (.bin or .gguf).';
      statusDiv.style.color = 'red';
      return;
    }
  } catch (_) {
    statusDiv.textContent = 'Please select a valid model file (.bin or .gguf).';
    statusDiv.style.color = 'red';
    return;
  }
  fs.writeFile(configPath, JSON.stringify(newConfig, null, 2), (err) => {
    if (err) {
      statusDiv.textContent = 'Failed to save: ' + err.message;
    } else {
      statusDiv.textContent = 'Saved!';
      // Notify main process to restart/reload backend
      try {
        window?.require && ipcRenderer.send('config-updated');
      } catch (_) {}
      // Close the window after applying
      setTimeout(() => { try { window.close(); } catch (_) {} }, 150);
    }
  });
};

defaultBtn.onclick = (e) => {
  e.preventDefault();
  try {
    let defaultConfigPath;
    if (process.env.APPIMAGE) {
      const appImageMountPath = getAppImageMountPath();
      defaultConfigPath = path.join(appImageMountPath, 'usr', 'share', 'coral', 'conf', 'config.json');
    } else {
      const platformConfig = process.platform === 'win32' ? 'config.json' : 'config-linux.json';
      defaultConfigPath = path.join(__dirname, '../..', 'coral', 'conf', platformConfig);
    }
    const data = fs.readFileSync(defaultConfigPath, 'utf8');
    config = JSON.parse(data);
    // Write defaults into user config and re-render form
    fs.writeFile(configPath, JSON.stringify(config, null, 2), (err) => {
      if (err) {
        statusDiv.textContent = 'Failed to set defaults: ' + err.message;
      } else {
        renderForm(config);
        statusDiv.textContent = 'Defaults applied';
        try { ipcRenderer.send('config-updated'); } catch (_) {}
      }
    });
  } catch (ex) {
    statusDiv.textContent = 'Failed to load defaults: ' + (ex && ex.message ? ex.message : ex);
  }
};

loadConfig(); 