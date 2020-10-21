# v380-nodejs

### How to run
1. Download & Install nodejs
2. Go to this directory and execute `npm install`
3. Create config.json file by following config.example.json (currently IP is not auto discoverable yet, so need to specify it)
4. Run via this command `npm run start` or `npm run dev2`
5. Connect to stream via URL `http://localhost:4000/video/stream.flv` (port can be changed inside config.json)

### Status
Currently this is buggy:
- The camera stream should start when client first connect to it, but currently it is started before client connected to it. The client will lose I-Frame, a workaround has been implemented for this.
- The audio codec is selected by guesswork.
