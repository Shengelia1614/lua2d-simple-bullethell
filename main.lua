local PlayingState = require('states.playing')
local GameOverState = require('states.gameover')

-- game logic is as follows
-- you there is an enemy moving around the screen bouncing off walls and shooting bullets at the player
-- the player can move around and deploy a barrier that bounces the enemy bullets away if it collides with it
-- since player is faster than the enemy if you deplay barier and run into him you may trigger colision even tho there is a barrier
-- there is no winning  your gaol is to get highest survival time,
-- at the start bullets can only bounce 3 times
-- every 30 seconds the max bounces increase by 1

local states = {
    playing = PlayingState,
    gameover = GameOverState
}

local currentState = nil
local currentStateName = nil

local function changeState(stateName)
    if currentState and currentState.exit then
        currentState:exit()
    end

    currentStateName = stateName
    currentState = states[stateName]

    if currentState and currentState.enter then
        currentState:enter()
    end
end

local VIRTUAL_WIDTH, VIRTUAL_HEIGHT = 1280, 720

-- Canvas for rendering at virtual resolution
local canvas
local canvasScale = 1
local canvasOffsetX = 0
local canvasOffsetY = 0

local function updateCanvasScale()
    local windowWidth = love.graphics.getWidth()
    local windowHeight = love.graphics.getHeight()

    -- Calculate scale to fit window while maintaining aspect ratio
    local scaleX = windowWidth / VIRTUAL_WIDTH
    local scaleY = windowHeight / VIRTUAL_HEIGHT
    canvasScale = math.min(scaleX, scaleY)

    -- Calculate offsets to center the canvas (letterboxing)
    canvasOffsetX = (windowWidth - (VIRTUAL_WIDTH * canvasScale)) / 2
    canvasOffsetY = (windowHeight - (VIRTUAL_HEIGHT * canvasScale)) / 2
end

function love.load()
    love.window.setTitle('Bullet Hell')
    love.window.setMode(1280, 720, {
        resizable = true,
        vsync = true
    })

    -- Create canvas at virtual resolution
    canvas = love.graphics.newCanvas(VIRTUAL_WIDTH, VIRTUAL_HEIGHT)
    canvas:setFilter('nearest', 'nearest') -- Pixel-perfect scaling

    updateCanvasScale()
    math.randomseed(os.time())
    changeState('playing')
end

function love.resize(w, h)
    updateCanvasScale()
end

function love.update(dt)
    if currentState and currentState.update then
        local nextState = currentState:update(dt)
        if nextState then
            changeState(nextState)
        end
    end
end

function love.draw()
    -- Draw everything to the virtual canvas
    love.graphics.setCanvas(canvas)
    love.graphics.clear(0.1, 0.1, 0.15)

    if currentState and currentState.draw then
        currentState:draw()
    end

    love.graphics.setCanvas()

    -- Draw the canvas scaled to the window
    love.graphics.clear(0, 0, 0) -- Black letterbox bars
    love.graphics.setColor(1, 1, 1)
    love.graphics.draw(canvas, canvasOffsetX, canvasOffsetY, 0, canvasScale, canvasScale)
end
