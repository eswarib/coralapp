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
  const userConfigPath = path.join(userConfigDir, 'config.json');
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
  // Development fallback
  configPath = path.join(__dirname, '../..', 'coral', 'conf', 'config.json');
}
const form = document.getElementById('configForm');
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

function renderForm(cfg) {
  form.innerHTML = '';
  for (const [key, value] of Object.entries(cfg)) {
    const label = document.createElement('label');
    label.textContent = key;
    let input;
    if (key === 'whisperModelPath') {
      input = document.createElement('input');
      input.type = 'text';
      input.value = value;
      input.name = key;
      input.id = 'whisperModelPathInput';
      label.appendChild(input);
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.textContent = 'Select File';
      btn.onclick = () => {
        ipcRenderer.invoke('select-model-file').then(filePath => {
          if (filePath) {
            input.value = filePath;
          }
        });
      };
      label.appendChild(btn);
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
    form.appendChild(label);
  }
}

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
      renderForm(config);
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
  const newConfig = {};
  for (const el of form.elements) {
    if (!el.name) continue;
    if (el.tagName === 'SELECT') {
      newConfig[el.name] = el.value === 'true';
    } else if (el.type === 'number') {
      newConfig[el.name] = Number(el.value);
    } else {
      newConfig[el.name] = el.value;
    }
  }
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
      // Dev path to default
      defaultConfigPath = path.join(__dirname, '../..', 'coral', 'conf', 'config.json');
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