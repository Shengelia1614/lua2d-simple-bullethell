local MenuState = {}

local VIRTUAL_WIDTH, VIRTUAL_HEIGHT = 1280, 720

function MenuState:enter()
    self.selectedIndex = 1
    self.audioFiles = {}
    self.startButtonHovered = false

    -- Scan notes/audio directory for audio files
    local audioDir = "notes/audio"
    local files = love.filesystem.getDirectoryItems(audioDir)

    for _, file in ipairs(files) do
        local ext = file:match("%.([^%.]+)$")
        if ext and (ext:lower() == "mp3" or ext:lower() == "wav" or ext:lower() == "ogg" or ext:lower() == "flac") then
            table.insert(self.audioFiles, file)
        end
    end

    -- Sort files alphabetically
    table.sort(self.audioFiles)

    -- If no audio files found, add a placeholder
    if #self.audioFiles == 0 then
        table.insert(self.audioFiles, "No audio files found")
    end

    -- Button dimensions
    self.startButton = {
        x = VIRTUAL_WIDTH / 2 - 100,
        y = VIRTUAL_HEIGHT - 150,
        width = 200,
        height = 60
    }

    -- List area
    self.listY = 200
    self.itemHeight = 40
    self.scrollOffset = 0
end

function MenuState:update(dt)
    -- Check mouse position for start button hover
    local mouseX, mouseY = love.mouse.getPosition()

    -- Convert screen coordinates to virtual coordinates
    local windowWidth = love.graphics.getWidth()
    local windowHeight = love.graphics.getHeight()
    local scaleX = windowWidth / VIRTUAL_WIDTH
    local scaleY = windowHeight / VIRTUAL_HEIGHT
    local scale = math.min(scaleX, scaleY)
    local offsetX = (windowWidth - (VIRTUAL_WIDTH * scale)) / 2
    local offsetY = (windowHeight - (VIRTUAL_HEIGHT * scale)) / 2

    local virtualMouseX = (mouseX - offsetX) / scale
    local virtualMouseY = (mouseY - offsetY) / scale

    -- Check if mouse is over start button
    self.startButtonHovered = virtualMouseX >= self.startButton.x and virtualMouseX <= self.startButton.x +
                                  self.startButton.width and virtualMouseY >= self.startButton.y and virtualMouseY <=
                                  self.startButton.y + self.startButton.height

    return nil
end

function MenuState:draw()
    love.graphics.setColor(1, 1, 1)

    -- Title
    love.graphics.printf("BULLET HELL", 0, 50, VIRTUAL_WIDTH, "center")
    love.graphics.printf("Select Music", 0, 100, VIRTUAL_WIDTH, "center")

    -- Draw audio file list
    for i, file in ipairs(self.audioFiles) do
        local y = self.listY + (i - 1) * self.itemHeight - self.scrollOffset

        -- Only draw if visible
        if y >= self.listY and y < VIRTUAL_HEIGHT - 200 then
            if i == self.selectedIndex then
                -- Highlight selected item
                love.graphics.setColor(0.3, 0.6, 1.0)
                love.graphics.rectangle('fill', VIRTUAL_WIDTH / 2 - 300, y, 600, self.itemHeight - 5)
                love.graphics.setColor(1, 1, 1)
            end

            -- Draw file name (without extension for cleaner look)
            local displayName = file:match("(.+)%.[^%.]+$") or file
            love.graphics.printf(displayName, VIRTUAL_WIDTH / 2 - 290, y + 10, 580, "left")
        end
    end

    -- Draw start button
    if self.startButtonHovered then
        love.graphics.setColor(0.4, 0.8, 0.4)
    else
        love.graphics.setColor(0.2, 0.6, 0.2)
    end
    love.graphics.rectangle('fill', self.startButton.x, self.startButton.y, self.startButton.width,
        self.startButton.height)

    love.graphics.setColor(1, 1, 1)
    love.graphics.rectangle('line', self.startButton.x, self.startButton.y, self.startButton.width,
        self.startButton.height)

    love.graphics.printf("START", self.startButton.x, self.startButton.y + 20, self.startButton.width, "center")

    -- Instructions
    love.graphics.setColor(0.7, 0.7, 0.7)
    love.graphics.printf("Use UP/DOWN arrows or click to select music", 0, VIRTUAL_HEIGHT - 80, VIRTUAL_WIDTH, "center")
    love.graphics.printf("Click START or press ENTER to begin", 0, VIRTUAL_HEIGHT - 50, VIRTUAL_WIDTH, "center")

    love.graphics.setColor(1, 1, 1)
end

function MenuState:keypressed(key)
    if key == 'up' then
        self.selectedIndex = math.max(1, self.selectedIndex - 1)

        -- Scroll if needed
        local selectedY = self.listY + (self.selectedIndex - 1) * self.itemHeight - self.scrollOffset
        if selectedY < self.listY then
            self.scrollOffset = self.scrollOffset - self.itemHeight
        end

    elseif key == 'down' then
        self.selectedIndex = math.min(#self.audioFiles, self.selectedIndex + 1)

        -- Scroll if needed
        local selectedY = self.listY + (self.selectedIndex - 1) * self.itemHeight - self.scrollOffset
        if selectedY + self.itemHeight > VIRTUAL_HEIGHT - 200 then
            self.scrollOffset = self.scrollOffset + self.itemHeight
        end

    elseif key == 'return' or key == 'space' then
        return self:startGame()
    end

    return nil
end

function MenuState:mousepressed(x, y, button)
    if button == 1 then
        -- Convert screen coordinates to virtual coordinates
        local windowWidth = love.graphics.getWidth()
        local windowHeight = love.graphics.getHeight()
        local scaleX = windowWidth / VIRTUAL_WIDTH
        local scaleY = windowHeight / VIRTUAL_HEIGHT
        local scale = math.min(scaleX, scaleY)
        local offsetX = (windowWidth - (VIRTUAL_WIDTH * scale)) / 2
        local offsetY = (windowHeight - (VIRTUAL_HEIGHT * scale)) / 2

        local virtualX = (x - offsetX) / scale
        local virtualY = (y - offsetY) / scale

        -- Check if clicked on start button
        if virtualX >= self.startButton.x and virtualX <= self.startButton.x + self.startButton.width and virtualY >=
            self.startButton.y and virtualY <= self.startButton.y + self.startButton.height then
            return self:startGame()
        end

        -- Check if clicked on a list item
        for i, file in ipairs(self.audioFiles) do
            local itemY = self.listY + (i - 1) * self.itemHeight - self.scrollOffset

            if virtualX >= VIRTUAL_WIDTH / 2 - 300 and virtualX <= VIRTUAL_WIDTH / 2 + 300 and virtualY >= itemY and
                virtualY <= itemY + self.itemHeight then
                self.selectedIndex = i
                break
            end
        end
    end

    return nil
end

function MenuState:startGame()
    if #self.audioFiles > 0 and self.audioFiles[1] ~= "No audio files found" then
        -- Store selected audio file globally so bossfight can access it
        _G.selectedAudioFile = self.audioFiles[self.selectedIndex]
        return 'bossfight_1'
    end
    return nil
end

function MenuState:exit()
end

return MenuState
