local Player = require('entities.player')
local Enemy = require('entities.enemy')
local Barrier = require('entities.barrier')
local Collision = require('utils.collision')

local BossFight1State = {}

local VIRTUAL_WIDTH, VIRTUAL_HEIGHT = 1280, 720

-- Piano visualization settings
local PIANO_KEYS = 88 -- Standard piano (MIDI 21-108)
local PIANO_WIDTH = VIRTUAL_WIDTH * 0.3 -- 30% of screen width
local WHITE_KEY_HEIGHT = 50 -- Height of white keys
local BLACK_KEY_HEIGHT = 32 -- Height of black keys (shorter)
local PIANO_Y = 20 -- Y position (top of screen)
local PIANO_ARCH_DEPTH = 80 -- How deep the arch curves downward

-- Piano key pattern: true = white key, false = black key
-- Pattern repeats every 12 semitones: C, C#, D, D#, E, F, F#, G, G#, A, A#, B
local KEY_PATTERN = {true, false, true, false, true, true, false, true, false, true, false, true}

-- Boss behavior mode toggle
local BOSS_MODE_PIANO_SPREAD = true -- false = target player with shotgun, true = spread across piano arch

local function decodeJSON(str)
    -- Remove whitespace to simplify simple pattern searches (keeps parser small and dependency-free)
    str = str:gsub("%s+", "")

    -- Find the events array
    local eventsStart = str:find('"events":%[')
    if not eventsStart then
        return nil, "Could not find events array"
    end

    local events = {}
    local currentPos = eventsStart + 10 -- skip past "events":[

    while true do
        -- Find next event object
        local objStart = str:find("{", currentPos)
        if not objStart then
            break
        end

        local objEnd = str:find("}", objStart)
        if not objEnd then
            break
        end

        local objStr = str:sub(objStart, objEnd)

        -- Extract the new fields: time (float), midi_number (int), and duration if present.
        -- Keep backwards compatibility with older key 'midi'.
        local time = objStr:match('"time":([%d%.%-]+)')
        local midi_num = objStr:match('"midi_number":(%d+)') or objStr:match('"midi":(%d+)')
        local duration = objStr:match('"duration":([%d%.%-]+)')
        local velocity = objStr:match('"velocity":([%d%.%-]+)')
        local note_on_str = objStr:match('"note_on":(true|false)')

        -- If note_on exists, convert to boolean; if missing assume true (backwards compatible)
        local note_on = true
        if note_on_str == 'true' then
            note_on = true
        elseif note_on_str == 'false' then
            note_on = false
        end

        if time and midi_num then
            local ev = {
                time = tonumber(time),
                midi = tonumber(midi_num), -- keep key 'midi' because game logic expects event.midi
                velocity = tonumber(velocity)
            }
            if duration then
                ev.duration = tonumber(duration)
            end
            if note_on_str then
                ev.note_on = note_on
            end
            table.insert(events, ev)
        end

        currentPos = objEnd + 1

        -- Check if we've reached the end of the array
        if str:sub(currentPos, currentPos) == ']' then
            break
        end
    end

    -- Sort events by time to ensure chronological order
    table.sort(events, function(a, b)
        return a.time < b.time
    end)

    return {
        events = events
    }
end

local VIRTUAL_WIDTH, VIRTUAL_HEIGHT = 1280, 720

-- Custom bullet class for bossfight with curved trajectory
local BossBullet = {}
BossBullet.__index = BossBullet

-- Load sprites once (class-level)
BossBullet.sprites = nil
BossBullet.animationSets = nil

function BossBullet.loadSprites()
    if BossBullet.sprites then
        return
    end

    BossBullet.sprites = {}
    -- load sprites 1-20
    for i = 1, 20 do
        local path = string.format("sprites/projectile/LightEffect_%02d.png", i)
        BossBullet.sprites[i] = love.graphics.newImage(path)
    end

    -- define 4 animation sets
    BossBullet.animationSets = {{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}, {16, 17, 18, 19, 20}}
end

function BossBullet.new(x, y, targetX, targetY, midi, velocity, colorscheme)
    BossBullet.loadSprites()

    local self = setmetatable({}, BossBullet)
    self.x = x
    self.y = y

    -- MIDI value is 1-88, map to size scale 2.5 to 1.0 (reversed: lower MIDI = bigger)
    local midiClamped = math.max(1, math.min(88, midi))
    -- Invert the scale: when MIDI=1, scaleFactor=2.5; when MIDI=88, scaleFactor=1.0
    local scaleFactor = 3 - ((midiClamped - 1) / (88 - 1) * 2) -- 2.5 to 1.0

    -- Base dimensions scaled by MIDI
    local baseSize = 8
    self.width = baseSize * scaleFactor
    self.height = baseSize * scaleFactor

    -- Base speed scaled by MIDI
    local baseSpeed = 130
    self.baseSpeed = baseSpeed * scaleFactor
    self.speed = self.baseSpeed

    -- Velocity adds a temporary speed boost that decays
    self.velocityBoost = (velocity / 127) * self.baseSpeed -- boost based on velocity
    self.velocityDecayRate = 4.0 -- exponential decay factor (higher = faster decay)
    self.velocityLifetime = 0 -- tracks how long velocity boost has been active

    -- Arc parameters based on MIDI (unclamped for more dramatic arcs)
    -- Higher MIDI = bigger arc
    -- DISABLED: Arc movement (keeping for reference)
    -- self.arcIntensity = midi / 44 -- normalized, higher values = more arc

    -- Calculate initial direction towards target
    local dx = targetX - x
    local dy = targetY - y
    local distance = math.sqrt(dx * dx + dy * dy)

    -- Normalize direction
    if distance > 0 then
        dx = dx / distance
        dy = dy / distance
    end

    -- Store target for arc calculation
    self.targetX = targetX
    self.targetY = targetY
    self.startX = x
    self.startY = y

    -- Calculate perpendicular direction for arc
    -- DISABLED: Arc movement (keeping for reference)
    -- self.perpX = -dy -- perpendicular to initial direction
    -- self.perpY = dx

    -- set colour: vary brightness and saturation based on midi/velocity
    -- colorscheme is a base hue (0-360 degrees)
    local hue = colorscheme / 360.0 -- normalize to 0-1 range

    -- Saturation varies with MIDI pitch (higher pitch = more saturated)
    local saturation = 0.4 + (midi / 128) * 0.6 -- range: 0.4 to 1.0

    -- Brightness/value varies with velocity (louder = brighter)
    local value = 0.5 + (velocity / 127) * 0.5 -- range: 0.5 to 1.0

    -- Alpha based on velocity
    local alpha = 0.6 + (velocity / 127) * 0.4

    -- Convert HSV to RGB
    self.color = self:hsvToRgb(hue, saturation, value, alpha)

    -- Set initial velocity (linear movement)
    -- Apply initial boost from velocity
    local initialSpeed = self.baseSpeed + self.velocityBoost
    self.vx = dx * initialSpeed
    self.vy = dy * initialSpeed

    -- DISABLED: Arc progress (keeping for reference)
    -- Arc progress (0 to 1)
    -- self.arcProgress = 0
    -- self.arcSpeed = 0.8 -- how fast we traverse the arc

    self.active = true
    self.maxBounces = 4
    self.bounceCount = 0

    -- animation
    self.animationSet = BossBullet.animationSets[love.math.random(1, 4)]
    self.animationSequence = {1, 2, 3, 4, 5, 4, 3, 2}
    self.animationIndex = 1
    self.animationTimer = 0
    self.animationSpeed = 0.08
    self.scale = 2.7 * scaleFactor -- scale visual size too

    return self
end

function BossBullet:hsvToRgb(h, s, v, a)
    local r, g, b

    local i = math.floor(h * 6)
    local f = h * 6 - i
    local p = v * (1 - s)
    local q = v * (1 - f * s)
    local t = v * (1 - (1 - f) * s)

    i = i % 6

    if i == 0 then
        r, g, b = v, t, p
    elseif i == 1 then
        r, g, b = q, v, p
    elseif i == 2 then
        r, g, b = p, v, t
    elseif i == 3 then
        r, g, b = p, q, v
    elseif i == 4 then
        r, g, b = t, p, v
    elseif i == 5 then
        r, g, b = v, p, q
    end

    return {r, g, b, a or 1}
end

function BossBullet:update(dt, gameWidth, gameHeight, playerX, playerY)
    if not self.active then
        return
    end

    self.animationTimer = self.animationTimer + dt
    if self.animationTimer >= self.animationSpeed then
        self.animationTimer = self.animationTimer - self.animationSpeed
        self.animationIndex = self.animationIndex + 1
        if self.animationIndex > #self.animationSequence then
            self.animationIndex = 1
        end
    end

    -- Decay velocity boost over time using exponential decay
    if self.velocityBoost > 0 then
        self.velocityLifetime = self.velocityLifetime + dt

        -- Exponential decay formula: boost = initialBoost * e^(-decayRate * time)
        -- This creates a dramatic initial drop that slows down over time
        local initialBoost = (self.baseSpeed) -- reconstruct initial boost
        self.velocityBoost = initialBoost * math.exp(-self.velocityDecayRate * self.velocityLifetime)

        -- Stop tracking when boost becomes negligible
        if self.velocityBoost < 0.5 then
            self.velocityBoost = 0
        end

        -- Update speed with current boost
        self.speed = self.baseSpeed + self.velocityBoost

        -- Recalculate velocity to maintain direction but with new speed
        local currentMag = math.sqrt(self.vx * self.vx + self.vy * self.vy)
        if currentMag > 0 then
            local dirX = self.vx / currentMag
            local dirY = self.vy / currentMag
            self.vx = dirX * self.speed
            self.vy = dirY * self.speed
        end
    end

    -- DISABLED: Arc movement (keeping for reference)
    -- Update arc progress
    -- self.arcProgress = self.arcProgress + self.arcSpeed * dt

    -- Calculate curved trajectory based on initial direction
    -- The arc is applied perpendicular to the base direction
    -- This creates a curved path that maintains its shape
    -- local baseVx = self.vx
    -- local baseVy = self.vy

    -- Get normalized base direction
    -- local baseMag = math.sqrt(baseVx * baseVx + baseVy * baseVy)
    -- local baseDirX = baseVx / baseMag
    -- local baseDirY = baseVy / baseMag

    -- Apply arc using sine wave on perpendicular axis
    -- local arcFactor = math.sin(self.arcProgress * math.pi * 2) * self.arcIntensity

    -- Add perpendicular component for the curve
    -- local curvedVx = baseDirX + self.perpX * arcFactor
    -- local curvedVy = baseDirY + self.perpY * arcFactor

    -- Normalize and apply speed
    -- local curvedMag = math.sqrt(curvedVx * curvedVx + curvedVy * curvedVy)
    -- if curvedMag > 0 then
    --     curvedVx = (curvedVx / curvedMag) * self.speed
    --     curvedVy = (curvedVy / curvedMag) * self.speed
    -- end

    -- update position with curved velocity
    -- self.x = self.x + curvedVx * dt
    -- self.y = self.y + curvedVy * dt

    -- Linear movement: update position with constant velocity
    self.x = self.x + self.vx * dt
    self.y = self.y + self.vy * dt

    local bounced = false

    -- Wall bouncing
    if self.x < 0 then
        self.x = 0
        self.vx = math.abs(self.vx)
        bounced = true
    elseif self.x + self.width > gameWidth then
        self.x = gameWidth - self.width
        self.vx = -math.abs(self.vx)
        bounced = true
    end

    if self.y < 0 then
        self.y = 0
        self.vy = math.abs(self.vy)
        bounced = true
    elseif self.y + self.height > gameHeight then
        self.y = gameHeight - self.height
        self.vy = -math.abs(self.vy)
        bounced = true
    end

    -- count bounces
    if bounced then
        self.bounceCount = self.bounceCount + 1
        if self.bounceCount > self.maxBounces then
            self.active = false
        end
        -- Recalculate perpendicular for new direction after bounce
        local mag = math.sqrt(self.vx * self.vx + self.vy * self.vy)
        if mag > 0 then
            local normVx = self.vx / mag
            local normVy = self.vy / mag
            self.perpX = -normVy
            self.perpY = normVx
        end
        -- DISABLED: Arc progress reset (keeping for reference)
        -- Reset arc progress to maintain curve after bounce
        -- self.arcProgress = 0
    end
end

function BossBullet:draw()
    if not self.active then
        return
    end

    local frameIndex = self.animationSequence[self.animationIndex]
    local spriteIndex = self.animationSet[frameIndex]
    local sprite = BossBullet.sprites[spriteIndex]

    if sprite then
        local centerX = self.x + self.width / 2
        local centerY = self.y + self.height / 2

        local spriteWidth = sprite:getWidth()
        local spriteHeight = sprite:getHeight()
        local targetSize = math.max(self.width, self.height) * self.scale
        local scaleX = targetSize / spriteWidth
        local scaleY = targetSize / spriteHeight
        local finalScale = math.min(scaleX, scaleY)

        love.graphics.setColor(self.color[1], self.color[2], self.color[3], self.color[4])
        love.graphics.draw(sprite, centerX, centerY, 0, finalScale, finalScale, spriteWidth / 2, spriteHeight / 2)
    else
        love.graphics.setColor(1, 0, 0)
        love.graphics.rectangle('fill', self.x, self.y, self.width, self.height)
    end

    love.graphics.setColor(1, 1, 1)
end

function BossBullet:isActive()
    return self.active
end

function BossBullet:getCollisionBox()
    return self.x, self.y, self.width, self.height
end

-- BossFight1 State Functions
function BossFight1State:enter()
    -- Determine selected track (menu now sets _G.selectedTrackFile). For backward compatibility also check _G.selectedAudioFile
    local selectedFile = _G.selectedTrackFile or _G.selectedAudioFile or "Erik Satie - Gnossienne No. 1.mp3"

    -- Remove extension to get base name
    local baseName = selectedFile:match("(.+)%.[^%.]+$") or selectedFile

    -- Build note JSON path using the naming scheme: <base>_notes.json in notes/note_data
    local notePath = "notes/note_data/" .. baseName .. "_notes.json"

    -- Load note data
    self.noteEvents = {}
    local fileContents = love.filesystem.read(notePath)

    -- Choose color scheme for this bossfight: random hue (0-360 degrees)
    -- Examples: 0=red, 60=yellow, 120=green, 180=cyan, 240=blue, 300=magenta
    self.colorscheme = love.math.random(0, 360)
    print("Color scheme hue: " .. self.colorscheme .. "°")

    if fileContents then
        local data, err = decodeJSON(fileContents)
        if data and data.events then
            self.noteEvents = data.events
            print("Loaded " .. #self.noteEvents .. " note events from " .. notePath)
        else
            print("Error parsing JSON: " .. tostring(err))
        end
    else
        print("Could not load notes file: " .. notePath)
    end

    -- Load or lazily prepare note audio sources. Guard against environments
    -- where the audio subsystem or love.audio.newSource might not be available
    -- (prevents runtime errors such as "attempt to call a number value").
    self.notes = {}
    for i = 21, 108 do
        self.notes[i] = love.audio.newSource("notes/keys/" .. tostring(i - 20) .. ".mp3", "static")
        self.notes[i]:setLooping(false)
    end

    -- Initialize player
    self.player = Player.new(VIRTUAL_WIDTH / 2 - 10, VIRTUAL_HEIGHT / 2 - 10)

    -- Initialize boss enemy at top middle of screen with slow movement
    local bossX = VIRTUAL_WIDTH / 2 - 30 -- center horizontally (60 is enemy width)
    local bossY = 10 -- top of screen with some padding
    -- Slow movement: 0.4x normal speed (normal is vx=150, vy=100)
    self.boss = Enemy.new(bossX, bossY, 60, 40)

    -- Store initial boss position for piano spread mode
    self.bossInitialX = bossX + 30 -- center of boss (width/2)
    self.bossInitialY = bossY + 20 -- center of boss (height/2)

    self.barrier = Barrier.new(self.player)
    self.bullets = {}
    self.gameTime = 0
    self.currentEventIndex = 1

    -- Add a start delay before bullets begin spawning (in seconds)
    self.startDelay = 4.0 -- 2 second grace period at the start

    -- Track recent bullet spawn times for shotgun spread effect
    self.lastBulletTime = -999
    self.bulletClusterCount = 0

    -- Piano visualization: track active keys with fade-out effect
    -- Each key stores {active = bool, fadeTimer = number}
    self.pianoKeys = {}
    for i = 1, PIANO_KEYS do
        self.pianoKeys[i] = {
            active = false,
            fadeTimer = 0,
            fadeDuration = 0.3 -- How long the key stays lit after being pressed
        }
    end
end

function BossFight1State:update(dt)
    -- update game time
    self.gameTime = self.gameTime + dt

    -- Update player
    self.player:update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT)

    -- Update boss only in player-targeting mode
    if not BOSS_MODE_PIANO_SPREAD then
        self.boss:update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT)
    else
        -- Keep boss stationary in piano spread mode
        self.boss.x = self.bossInitialX - 30
        self.boss.y = self.bossInitialY - 20
    end

    -- Update barrier
    self.barrier:update(dt)

    -- Check for spacebar to deploy barrier
    if love.keyboard.isDown('space') then
        self.barrier:deploy()
    end

    if love.keyboard.isDown('lshift') then
        self.player.speed = self.player.baseSpeed * 0.5
    else
        self.player.speed = self.player.baseSpeed
    end

    -- Spawn bullets based on note events
    -- Each event has a "time" field that determines when (in seconds)
    -- the bullet should spawn after the bossfight starts
    -- Account for start delay: bullets only spawn after the grace period
    while self.currentEventIndex <= #self.noteEvents do
        local event = self.noteEvents[self.currentEventIndex]

        -- Check if enough time has passed to spawn this bullet (accounting for start delay)
        if self.gameTime >= (event.time + self.startDelay) then
            -- Spawn bullet from boss towards player
            local bossX, bossY = self.boss:getCenter()
            local playerX = self.player.x + self.player.width / 2
            local playerY = self.player.y + self.player.height / 2

            local dx, dy, targetX, targetY

            if BOSS_MODE_PIANO_SPREAD then
                -- Piano spread mode: shoot in direction based on note position (1-88)
                -- Map MIDI note (21-108) to angle spread (150 degrees total, centered downward)
                -- Angles: -75° to +75° from vertical (90° is straight down)

                local noteIndex = event.midi - 20 -- Convert MIDI (21-108) to index (1-88)
                local normalizedPos = (noteIndex - 1) / (PIANO_KEYS - 1) -- 0 to 1

                -- Map to angle: -75° to +75° (150° spread)
                -- Center is straight down (90° in standard coords, or PI/2 radians)
                -- REVERSED: lower notes (left) should shoot left, higher notes (right) should shoot right
                local spreadDegrees = 180
                local minAngle = 90 - (spreadDegrees / 2) -- 15°
                local maxAngle = 90 + (spreadDegrees / 2) -- 165°

                -- Reverse the mapping: 0 -> maxAngle (165°, left), 1 -> minAngle (15°, right)
                local angleDegrees = maxAngle - normalizedPos * spreadDegrees
                local angleRadians = math.rad(angleDegrees)

                -- Convert angle to direction vector
                dx = math.cos(angleRadians)
                dy = math.sin(angleRadians)

                -- Calculate target position
                targetX = bossX + dx * 1000
                targetY = bossY + dy * 1000
            else
                -- Player-targeting mode with shotgun spread
                local spreadAngle = 0
                local timeSinceLastBullet = event.time - self.lastBulletTime

                if timeSinceLastBullet <= 0.5 then
                    -- Part of a cluster - apply spread
                    self.bulletClusterCount = self.bulletClusterCount + 1

                    -- Spread pattern: alternate left/right with increasing angle
                    -- Max spread of ±30 degrees (±0.52 radians)
                    local maxSpread = math.pi / 6 -- 30 degrees
                    local spreadIndex = math.floor(self.bulletClusterCount / 2)
                    local spreadSide = (self.bulletClusterCount % 2 == 0) and 1 or -1

                    spreadAngle = spreadSide * (spreadIndex * 0.15) -- 0.15 radians (~8.6 degrees) per step
                    spreadAngle = math.max(-maxSpread, math.min(maxSpread, spreadAngle))
                else
                    -- New cluster - reset counter
                    self.bulletClusterCount = 0
                    spreadAngle = 0
                end

                self.lastBulletTime = event.time

                -- Calculate direction to player
                dx = playerX - bossX
                dy = playerY - bossY
                local distance = math.sqrt(dx * dx + dy * dy)

                -- Normalize
                if distance > 0 then
                    dx = dx / distance
                    dy = dy / distance
                end

                -- Apply spread rotation
                if spreadAngle ~= 0 then
                    local cos_angle = math.cos(spreadAngle)
                    local sin_angle = math.sin(spreadAngle)
                    local newDx = dx * cos_angle - dy * sin_angle
                    local newDy = dx * sin_angle + dy * cos_angle
                    dx = newDx
                    dy = newDy
                end

                -- Calculate target position for bullet constructor
                targetX = bossX + dx * 1000 -- project forward
                targetY = bossY + dy * 1000
            end

            -- Create bullet with MIDI-based properties
            local bullet = BossBullet.new(bossX, bossY, targetX, targetY, event.midi, event.velocity, self.colorscheme)

            -- Add bullet to active bullets list
            table.insert(self.bullets, bullet)

            -- key = love.audio.newSource("notes/keys/" .. tostring(event.midi - 21) .. ".mp3", "static")
            -- key:setLooping(false)
            -- key:setVolume(event.velocity / 127)
            -- index = #self.notes + 1
            -- table.insert(self.notes, key)
            -- self.notes[index]:play()

            self.notes[event.midi]:setVolume(event.velocity / 127)
            self.notes[event.midi]:stop()
            self.notes[event.midi]:play()

            -- Activate piano key visualization
            -- MIDI notes 21-108 map to piano keys 1-88
            local keyIndex = event.midi - 20 -- Convert MIDI (21-108) to key index (1-88)
            if keyIndex >= 1 and keyIndex <= PIANO_KEYS then
                self.pianoKeys[keyIndex].active = true
                self.pianoKeys[keyIndex].fadeTimer = self.pianoKeys[keyIndex].fadeDuration
            end

            self.currentEventIndex = self.currentEventIndex + 1
        else
            -- Haven't reached the time for this event yet, stop checking
            break
        end
    end

    -- Update piano key fade timers
    for i = 1, PIANO_KEYS do
        if self.pianoKeys[i].fadeTimer > 0 then
            self.pianoKeys[i].fadeTimer = self.pianoKeys[i].fadeTimer - dt
            if self.pianoKeys[i].fadeTimer <= 0 then
                self.pianoKeys[i].active = false
                self.pianoKeys[i].fadeTimer = 0
            end
        end
    end

    -- Update bullets
    for i = #self.bullets, 1, -1 do
        local bullet = self.bullets[i]
        bullet:update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT)

        -- check barrier collision for bullets
        if self.barrier:isActive() then
            local bx, by, bw, bh = bullet:getCollisionBox()
            if self.barrier:checkCollision(bx, by, bw, bh) then
                -- Bounce bullet away from barrier center
                local centerX, centerY = self.barrier:getCenter()
                local bulletCenterX = bx + bw / 2
                local bulletCenterY = by + bh / 2
                local dirX = bulletCenterX - centerX
                local dirY = bulletCenterY - centerY
                local magnitude = math.sqrt(dirX * dirX + dirY * dirY)
                if magnitude > 0 then
                    dirX = dirX / magnitude
                    dirY = dirY / magnitude
                    bullet.vx = dirX * bullet.speed
                    bullet.vy = dirY * bullet.speed
                end
            end
        end

        if not bullet:isActive() then
            table.remove(self.bullets, i)
        end
    end

    -- Check collisions
    local px, py, pw, ph = self.player:getCollisionBox()

    for _, bullet in ipairs(self.bullets) do
        if bullet:isActive() then
            local bx, by, bw, bh = bullet:getCollisionBox()

            if Collision.checkAABB(px, py, pw, ph, bx, by, bw, bh) then
                return 'gameover'
            end
        end
    end

    return nil
end

function BossFight1State:draw()
    self.player:draw()
    self.boss:draw()

    for _, bullet in ipairs(self.bullets) do
        bullet:draw()
    end

    -- draw barrier
    self.barrier:draw()

    love.graphics.setColor(1, 1, 1)
    love.graphics.print('BOSS FIGHT!', VIRTUAL_WIDTH / 2 - 40, 10)
    love.graphics.print('Time: ' .. math.floor(self.gameTime) .. 's', 10, 10)
    love.graphics.print('Bullets: ' .. #self.bullets, 10, 30)
    love.graphics.print('WASD/Arrows to move, SPACE for barrier', 10, 50)
    love.graphics.print('Barrier Uses: ' .. self.barrier:getUsesRemaining(), 10, 70)
    love.graphics.print('Notes: ' .. self.currentEventIndex .. '/' .. #self.noteEvents, 10, 90)

    -- Draw piano visualization last (top layer, above everything)
    self:drawPiano()
end

function BossFight1State:drawPiano()
    -- Calculate piano position (centered horizontally)
    local pianoStartX = (VIRTUAL_WIDTH - PIANO_WIDTH) / 2

    -- Count white keys to determine spacing
    local whiteKeyCount = 0
    for i = 1, PIANO_KEYS do
        local noteInOctave = ((i + 8) % 12) + 1 -- MIDI 21 starts at A0, offset by 9 semitones from C
        if KEY_PATTERN[noteInOctave] then
            whiteKeyCount = whiteKeyCount + 1
        end
    end

    local whiteKeyWidth = PIANO_WIDTH / whiteKeyCount
    local blackKeyWidth = whiteKeyWidth * 0.6 -- Black keys are narrower

    -- First pass: Draw all white keys
    local whiteKeyIndex = 0
    for i = 1, PIANO_KEYS do
        local noteInOctave = ((i + 8) % 12) + 1 -- MIDI 21 (A0) offset

        if KEY_PATTERN[noteInOctave] then
            -- Calculate X position for this white key
            local keyX = pianoStartX + whiteKeyIndex * whiteKeyWidth

            -- Calculate normalized position for arch (0 to 1 across all white keys)
            local normalizedPos = whiteKeyIndex / (whiteKeyCount - 1)

            -- Calculate Y position using arch curve (downward parabola)
            local archOffset = -PIANO_ARCH_DEPTH * 4 * math.pow(normalizedPos - 0.5, 2)
            local keyY = PIANO_Y + PIANO_ARCH_DEPTH + archOffset

            -- Determine key color based on active state
            local key = self.pianoKeys[i]

            if key.active and key.fadeTimer > 0 then
                -- Active white key: use color scheme
                local alpha = key.fadeTimer / key.fadeDuration
                local hue = self.colorscheme / 360.0
                local r, g, b = self:hsvToRgb(hue, 0.7, 1.0)
                love.graphics.setColor(r, g, b, alpha)
            else
                -- Inactive white key: light gray
                love.graphics.setColor(0.9, 0.9, 0.9, 1.0)
            end

            -- Draw white key
            love.graphics.rectangle('fill', keyX, keyY, whiteKeyWidth - 2, WHITE_KEY_HEIGHT)

            -- Draw outline (thin border)
            love.graphics.setColor(0.1, 0.1, 0.1, 1.0)
            love.graphics.setLineWidth(0.5)
            love.graphics.rectangle('line', keyX, keyY, whiteKeyWidth - 2, WHITE_KEY_HEIGHT)

            whiteKeyIndex = whiteKeyIndex + 1
        end
    end

    -- Second pass: Draw all black keys (on top of white keys)
    whiteKeyIndex = 0
    for i = 1, PIANO_KEYS do
        local noteInOctave = ((i + 8) % 12) + 1

        -- Track white key position for black key placement
        if KEY_PATTERN[noteInOctave] then
            whiteKeyIndex = whiteKeyIndex + 1
        else
            -- Black key: positioned between white keys
            -- Place it offset from the previous white key
            local keyX = pianoStartX + (whiteKeyIndex - 0.5) * whiteKeyWidth - blackKeyWidth / 2

            -- Use the same normalized position as nearby white key for arch
            local normalizedPos = (whiteKeyIndex - 0.5) / (whiteKeyCount - 1)
            normalizedPos = math.max(0, math.min(1, normalizedPos))

            local archOffset = -PIANO_ARCH_DEPTH * 4 * math.pow(normalizedPos - 0.5, 2)
            local keyY = PIANO_Y + PIANO_ARCH_DEPTH + archOffset

            -- Determine key color based on active state
            local key = self.pianoKeys[i]

            if key.active and key.fadeTimer > 0 then
                -- Active black key: use color scheme with higher saturation
                local alpha = key.fadeTimer / key.fadeDuration
                local hue = self.colorscheme / 360.0
                local r, g, b = self:hsvToRgb(hue, 0.9, 0.8)
                love.graphics.setColor(r, g, b, alpha)
            else
                -- Inactive black key: dark gray/black
                love.graphics.setColor(0.15, 0.15, 0.15, 1.0)
            end

            -- Draw black key (shorter than white keys)
            love.graphics.rectangle('fill', keyX, keyY, blackKeyWidth, BLACK_KEY_HEIGHT)

            -- Draw outline (thin border)
            love.graphics.setColor(0.05, 0.05, 0.05, 1.0)
            love.graphics.setLineWidth(0.5)
            love.graphics.rectangle('line', keyX, keyY, blackKeyWidth, BLACK_KEY_HEIGHT)
        end
    end

    -- Reset line width to default
    love.graphics.setLineWidth(1)
    love.graphics.setColor(1, 1, 1)
end

function BossFight1State:hsvToRgb(h, s, v)
    local r, g, b

    local i = math.floor(h * 6)
    local f = h * 6 - i
    local p = v * (1 - s)
    local q = v * (1 - f * s)
    local t = v * (1 - (1 - f) * s)

    i = i % 6

    if i == 0 then
        r, g, b = v, t, p
    elseif i == 1 then
        r, g, b = q, v, p
    elseif i == 2 then
        r, g, b = p, v, t
    elseif i == 3 then
        r, g, b = p, q, v
    elseif i == 4 then
        r, g, b = t, p, v
    elseif i == 5 then
        r, g, b = v, p, q
    end

    return r, g, b
end

function BossFight1State:exit()
    if self.notes then
        for _, src in pairs(self.notes) do
            if src and type(src.stop) == 'function' then
                -- stop any playing source
                src:stop()
            end
        end
    end
end

return BossFight1State
