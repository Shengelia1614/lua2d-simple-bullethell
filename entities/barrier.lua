local Barrier = {}
Barrier.__index = Barrier

-- load sprites once
Barrier.sprites = nil

function Barrier.loadSprites()
    if Barrier.sprites then
        return
    end

    Barrier.sprites = {}
    for i = 6, 10 do
        local path = string.format("sprites/barier/Cosmic_%02d.png", i)
        Barrier.sprites[i] = love.graphics.newImage(path)
    end
end

function Barrier.new(player)
    Barrier.loadSprites()

    local self = setmetatable({}, Barrier)
    self.player = player
    self.active = false
    self.deployDuration = 3.0 -- seconds
    self.timer = 0
    self.usesRemaining = 3 -- can only be used 3 times

    -- animation
    self.animationSpeed = 0.1 -- seconds per frame
    self.animationTimer = 0
    self.currentFrame = 6
    self.deploying = false
    self.retracting = false

    local playerSize = math.max(player.width, player.height)
    self.radius = (playerSize / 2) * 3

    return self
end

function Barrier:deploy()
    if not self.active and self.usesRemaining > 0 then
        self.active = true
        self.deploying = true
        self.retracting = false
        self.timer = 0
        self.currentFrame = 6
        self.animationTimer = 0
        self.usesRemaining = self.usesRemaining - 1
    end
end

function Barrier:update(dt)
    if not self.active then
        return
    end

    self.timer = self.timer + dt
    self.animationTimer = self.animationTimer + dt

    if self.deploying then
        if self.animationTimer >= self.animationSpeed then
            self.animationTimer = 0
            self.currentFrame = self.currentFrame + 1
            if self.currentFrame >= 10 then
                self.currentFrame = 10
                self.deploying = false
            end
        end
    end

    if self.timer >= self.deployDuration and not self.retracting then
        self.retracting = true
        self.deploying = false
        self.animationTimer = 0
    end

    if self.retracting then
        if self.animationTimer >= self.animationSpeed then
            self.animationTimer = 0
            self.currentFrame = self.currentFrame - 1
            if self.currentFrame <= 6 then
                self.currentFrame = 6
                self.active = false
                self.retracting = false
            end
        end
    end
end

function Barrier:draw()
    if not self.active then
        return
    end

    local sprite = Barrier.sprites[self.currentFrame]
    if sprite then
        local centerX = self.player.x + self.player.width / 2
        local centerY = self.player.y + self.player.height / 2

        local playerSize = math.max(self.player.width, self.player.height)
        local targetSize = playerSize * 6
        local spriteWidth = sprite:getWidth()
        local spriteHeight = sprite:getHeight()
        local spriteSize = math.max(spriteWidth, spriteHeight)
        local scale = targetSize / spriteSize

        love.graphics.setColor(1, 1, 1, 0.8)
        love.graphics.draw(sprite, centerX, centerY, 0, scale, scale, spriteWidth / 2, spriteHeight / 2)
        love.graphics.setColor(1, 1, 1)
    end
end

function Barrier:isActive()
    return self.active and self.currentFrame > 6
end

function Barrier:checkCollision(x, y, width, height)
    if not self:isActive() then
        return false
    end

    local centerX = self.player.x + self.player.width / 2
    local centerY = self.player.y + self.player.height / 2

    -- find closest point on the rectangle to the circle center
    local closestX = math.max(x, math.min(centerX, x + width))
    local closestY = math.max(y, math.min(centerY, y + height))

    -- calculate distance from circle center to closest point
    local distX = centerX - closestX
    local distY = centerY - closestY
    local distanceSquared = distX * distX + distY * distY

    return distanceSquared < (self.radius * self.radius)
end

function Barrier:getCenter()
    local centerX = self.player.x + self.player.width / 2
    local centerY = self.player.y + self.player.height / 2
    return centerX, centerY
end

function Barrier:getRadius()
    return self.radius
end

function Barrier:getUsesRemaining()
    return self.usesRemaining
end

return Barrier
