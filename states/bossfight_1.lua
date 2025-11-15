local Player = require('entities.player')
local Enemy = require('entities.enemy')
local Barrier = require('entities.barrier')
local Collision = require('utils.collision')

local BossFight1State = {}

local VIRTUAL_WIDTH, VIRTUAL_HEIGHT = 1280, 720

-- Simple JSON decoder for our specific use case
local function decodeJSON(str)
    -- Remove whitespace
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

        -- Extract fields
        local time = objStr:match('"time":([%d%.%-]+)')
        local note = objStr:match('"note":"([^"]+)"')
        local frequency = objStr:match('"frequency":([%d%.%-]+)')
        local magnitude = objStr:match('"magnitude":([%d%.%-]+)')
        local onset = objStr:match('"onset_strength":([%d%.%-]+)')
        local midi = objStr:match('"midi":(%d+)')

        if time and midi then
            table.insert(events, {
                time = tonumber(time),
                note = note,
                frequency = tonumber(frequency),
                magnitude = tonumber(magnitude),
                onset_strength = tonumber(onset),
                midi = tonumber(midi)
            })
        end

        currentPos = objEnd + 1

        -- Check if we've reached the end of the array
        if str:sub(currentPos, currentPos) == ']' then
            break
        end
    end

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

function BossBullet.new(x, y, targetX, targetY, midi)
    BossBullet.loadSprites()

    local self = setmetatable({}, BossBullet)
    self.x = x
    self.y = y

    -- MIDI value is 1-88, map to size scale 2.5 to 1.0 (reversed: lower MIDI = bigger)
    local midiClamped = math.max(1, math.min(88, midi))
    -- Invert the scale: when MIDI=1, scaleFactor=2.5; when MIDI=88, scaleFactor=1.0
    local scaleFactor = 2.5 - ((midiClamped - 1) / (88 - 1) * 1.5) -- 2.5 to 1.0

    -- Base dimensions scaled by MIDI
    local baseSize = 8
    self.width = baseSize * scaleFactor
    self.height = baseSize * scaleFactor

    -- Base speed scaled by MIDI (50% to 100%)
    local baseSpeed = 250
    self.speed = baseSpeed * scaleFactor

    -- Arc parameters based on MIDI (unclamped for more dramatic arcs)
    -- Higher MIDI = bigger arc
    self.arcIntensity = midi / 44 -- normalized, higher values = more arc

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
    self.perpX = -dy -- perpendicular to initial direction
    self.perpY = dx

    -- Set initial velocity
    self.vx = dx * self.speed
    self.vy = dy * self.speed

    -- Arc progress (0 to 1)
    self.arcProgress = 0
    self.arcSpeed = 0.8 -- how fast we traverse the arc

    self.active = true
    self.maxBounces = 3
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

    -- Update arc progress
    self.arcProgress = self.arcProgress + self.arcSpeed * dt

    -- Calculate curved trajectory based on initial direction
    -- The arc is applied perpendicular to the base direction
    -- This creates a curved path that maintains its shape
    local baseVx = self.vx
    local baseVy = self.vy

    -- Get normalized base direction
    local baseMag = math.sqrt(baseVx * baseVx + baseVy * baseVy)
    local baseDirX = baseVx / baseMag
    local baseDirY = baseVy / baseMag

    -- Apply arc using sine wave on perpendicular axis
    local arcFactor = math.sin(self.arcProgress * math.pi * 2) * self.arcIntensity

    -- Add perpendicular component for the curve
    local curvedVx = baseDirX + self.perpX * arcFactor
    local curvedVy = baseDirY + self.perpY * arcFactor

    -- Normalize and apply speed
    local curvedMag = math.sqrt(curvedVx * curvedVx + curvedVy * curvedVy)
    if curvedMag > 0 then
        curvedVx = (curvedVx / curvedMag) * self.speed
        curvedVy = (curvedVy / curvedMag) * self.speed
    end

    -- update position with curved velocity
    self.x = self.x + curvedVx * dt
    self.y = self.y + curvedVy * dt

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
        -- Reset arc progress to maintain curve after bounce
        self.arcProgress = 0
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

        love.graphics.setColor(1, 1, 1, 1)
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
    -- Get selected audio file from global variable (set by menu)
    local selectedAudio = _G.selectedAudioFile or "Erik Satie - Gnossienne No. 1.mp3"

    -- Convert audio filename to notes filename
    -- Remove extension and add "_notes.json"
    local audioNameWithoutExt = selectedAudio:match("(.+)%.[^%.]+$") or selectedAudio
    local notesFilename = audioNameWithoutExt .. "_notes.json"

    -- Build full paths
    local audioPath = "notes/audio/" .. selectedAudio
    local notePath = "notes/note_data/" .. notesFilename

    -- Load note data
    self.noteEvents = {}
    local fileContents = love.filesystem.read(notePath)

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

    -- Load and play music
    self.music = love.audio.newSource(audioPath, "stream")
    self.music:setLooping(false)
    self.music:play()
    print("Playing: " .. selectedAudio)

    -- Initialize player
    self.player = Player.new(VIRTUAL_WIDTH / 2 - 10, VIRTUAL_HEIGHT / 2 - 10)

    -- Initialize boss enemy at top middle of screen with slow movement
    local bossX = VIRTUAL_WIDTH / 2 - 30 -- center horizontally (60 is enemy width)
    local bossY = 50 -- top of screen with some padding
    -- Slow movement: 0.4x normal speed (normal is vx=150, vy=100)
    self.boss = Enemy.new(bossX, bossY, 60, 40)

    self.barrier = Barrier.new(self.player)
    self.bullets = {}
    self.gameTime = 0
    self.currentEventIndex = 1
end

function BossFight1State:update(dt)
    -- update game time
    self.gameTime = self.gameTime + dt

    -- Update player
    self.player:update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT)

    -- Update boss (slow movement)
    self.boss:update(dt, VIRTUAL_WIDTH, VIRTUAL_HEIGHT)

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
    while self.currentEventIndex <= #self.noteEvents do
        local event = self.noteEvents[self.currentEventIndex]

        -- Check if enough time has passed to spawn this bullet
        if self.gameTime >= event.time then
            -- Spawn bullet from boss towards player
            local bossX, bossY = self.boss:getCenter()
            local playerX = self.player.x + self.player.width / 2
            local playerY = self.player.y + self.player.height / 2

            -- Create bullet with MIDI-based properties
            local bullet = BossBullet.new(bossX, bossY, playerX, playerY, event.midi)
            table.insert(self.bullets, bullet)

            self.currentEventIndex = self.currentEventIndex + 1
        else
            -- Haven't reached the time for this event yet, stop checking
            break
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
end

function BossFight1State:exit()
    if self.music then
        self.music:stop()
    end
end

return BossFight1State
